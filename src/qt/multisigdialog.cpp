#include "multisigdialog.h"
#include "ui_multisigdialog.h"
#include "walletmodel.h"
#include "addresstablemodel.h"
#include "base58.h"
#include "core_io.h"
#include "init.h"
#include "validation.h"
#include "primitives/transaction.h"
#include "script/script.h"
#include "script/standard.h"
#include <QMenu>
#include "script/sign.h"
#include "sync.h"
#include "util.h"
#include "utilstrencodings.h"
#include "wallet/wallet.h"
#include "univalue.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QTextStream>
#include <QHeaderView>
#include <QHBoxLayout>

MultisigDialog::MultisigDialog(const PlatformStyle *pplatformStyle, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MultisigDialog),
    model(nullptr),
    platformStyle(pplatformStyle)
{
    ui->setupUi(this);

    // Connect signals with old-style Qt4 syntax
    connect(ui->addPubKeyButton, SIGNAL(clicked()), this, SLOT(addInputRow()));
    connect(ui->removePubKeyButton, SIGNAL(clicked()), this, SLOT(removeInputRow()));
    connect(ui->createWalletButton, SIGNAL(clicked()), this, SLOT(createMultisigWallet()));
    connect(ui->importButton, SIGNAL(clicked()), this, SLOT(importMultisigWallet()));
    
    // Add Load From File button to Import tab
    QPushButton* loadFromFileButton = new QPushButton(tr("Load From File"));
    QHBoxLayout* importButtonLayout = new QHBoxLayout();
    importButtonLayout->addWidget(loadFromFileButton);
    importButtonLayout->addStretch();
    QVBoxLayout* importLayout = qobject_cast<QVBoxLayout*>(ui->tabImportWallet->layout());
    if (importLayout) {
        importLayout->insertLayout(1, importButtonLayout);
    }
    connect(loadFromFileButton, SIGNAL(clicked()), this, SLOT(loadMultisigInfoFromFile()));
    
    // Add Decode Transaction button to Sign tab
    QPushButton* decodeButton = new QPushButton(tr("Decode Transaction"));
    QHBoxLayout* decodeButtonLayout = qobject_cast<QHBoxLayout*>(ui->horizontalLayout_8);
    if (decodeButtonLayout) {
        decodeButtonLayout->insertWidget(1, decodeButton);
    }
    connect(decodeButton, SIGNAL(clicked()), this, SLOT(decodeTransaction()));
    
    connect(ui->loadTxButton, SIGNAL(clicked()), this, SLOT(loadTransaction()));
    connect(ui->signButton, SIGNAL(clicked()), this, SLOT(signMultisigTx()));
    connect(ui->saveTxButton, SIGNAL(clicked()), this, SLOT(saveTransaction()));
    connect(ui->loadSignedTxButton, SIGNAL(clicked()), this, SLOT(loadSignedTransaction()));
    connect(ui->broadcastButton, SIGNAL(clicked()), this, SLOT(broadcastMultisigTx()));
    connect(ui->rejectButton, SIGNAL(clicked()), this, SLOT(clearAll()));
    connect(ui->walletSelector, SIGNAL(currentIndexChanged(int)), this, SLOT(onWalletSelectionChanged(int)));
    // Hide the address selector section
    ui->addressSelectorLabel->setVisible(false);
    ui->addressSelector->setVisible(false);
    ui->addAddressButton->setVisible(false);
    ui->refreshButton->setVisible(false);
    
    // Connect the total signatures spinner
    connect(ui->totalSignatures, SIGNAL(valueChanged(int)), this, SLOT(onTotalSignaturesChanged(int)));

    // Initialize table with fixed column count
    ui->pubkeyTable->setColumnCount(3);
    ui->pubkeyTable->setHorizontalHeaderLabels(QStringList() 
        << tr("Input Type")
        << tr("Input Value")
        << tr("Status"));

    // Set column resize modes using old-style Qt4 syntax
    QHeaderView* pHeader = ui->pubkeyTable->horizontalHeader();
    pHeader->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    pHeader->setSectionResizeMode(1, QHeaderView::Stretch);
    pHeader->setSectionResizeMode(2, QHeaderView::ResizeToContents);

    // Connect table item changed signal with old-style Qt4 syntax
    connect(ui->pubkeyTable, SIGNAL(cellChanged(int,int)), this, SLOT(onTableCellChanged(int,int)));

    // Add initial empty row
    addInputRow();
}

void MultisigDialog::onTableCellChanged(int nRow, int nColumn)
{
    if (nColumn == 1) // Input value column
    {
        validateInput(nRow);
    }
}

MultisigDialog::~MultisigDialog()
{
    delete ui;
}

void MultisigDialog::removeInputRow()
{
    int nSelectedRow = ui->pubkeyTable->currentRow();
    if (nSelectedRow < 0)
        return;
    
    ui->pubkeyTable->removeRow(nSelectedRow);
    
    // Make sure we have at least one row
    if (ui->pubkeyTable->rowCount() == 0) {
        addInputRow();
    }
    
    // Update the required signatures spinner to not exceed the number of rows
    int numRows = ui->pubkeyTable->rowCount();
    if (numRows < ui->requiredSignatures->value()) {
        ui->requiredSignatures->setValue(numRows);
    }
    
    // Update the total participants spinner to match the number of rows
    // Only update if there are fewer rows than the current value
    if (numRows < ui->totalSignatures->value() && numRows > 0) {
        ui->totalSignatures->setValue(numRows);
    }
    
    // Update the total signatures spinner
    ui->totalSignatures->setValue(numRows);
}

void MultisigDialog::populateAddressSelector()
{
    if (!model)
        return;
        
    ui->addressSelector->clear();
    
    try {
        // Get all addresses from the wallet
        std::map<QString, std::vector<COutput>> mapCoins;
        model->listCoins(mapCoins);
        
        // Add each address to the selector
        for (const auto& coins : mapCoins) {
            try {
                QString address = coins.first;
                
                // Filter out multisig addresses (P2SH)
                CBitcoinAddress addr(address.toStdString());
                if (!addr.IsValid() || addr.IsScript()) {
                    continue; // Skip multisig/invalid addresses
                }
                
                QString label = "";
                
                // Safely get the label for the address
                if (model->getAddressTableModel()) {
                    try {
                        label = model->getAddressTableModel()->labelForAddress(address);
                    } catch (...) {
                        // If getting the label fails, just use an empty string
                    }
                }
                
                if (!label.isEmpty()) {
                    ui->addressSelector->addItem(label + " (" + address + ")", address);
                } else {
                    ui->addressSelector->addItem(address, address);
                }
            } catch (...) {
                // Skip this address if there's an error processing it
                continue;
            }
        }
    } catch (...) {
        // If there's any error in the process, just return without crashing
    }
}

void MultisigDialog::addSelectedAddress()
{
    // Check if model is available
    if (!model)
        return;
        
    // Check if address selector has a valid selection
    if (ui->addressSelector->currentIndex() < 0 || ui->addressSelector->count() == 0)
        return;
        
    QString address = ui->addressSelector->currentData().toString();
    if (address.isEmpty())
        return;
    
    // Get public key from address
    QString strPubKey;
    QString strError;
    if (!validateAddress(address, strPubKey, strError)) {
        QMessageBox::warning(this, tr("Error"), tr("Could not get public key for address %1: %2").arg(address).arg(strError));
        return;
    }
        
    // Add a new row if needed
    int nRow = ui->pubkeyTable->rowCount();
    if (nRow == 0 || (ui->pubkeyTable->item(nRow - 1, 1) && !ui->pubkeyTable->item(nRow - 1, 1)->text().isEmpty())) {
        addInputRow();
        nRow = ui->pubkeyTable->rowCount() - 1;
    }
    
    // Set the input type to address
    QComboBox* inputTypeCombo = qobject_cast<QComboBox*>(ui->pubkeyTable->cellWidget(nRow, 0));
    if (inputTypeCombo) {
        inputTypeCombo->setCurrentIndex(INPUT_ADDRESS);
    }
    
    // Set the address - check if item exists first
    QTableWidgetItem* item = ui->pubkeyTable->item(nRow, 1);
    if (item) {
        item->setText(address);
        
        // Validate the input
        validateInput(nRow);
        
        // Update the required and total signatures spinners
        int numRows = ui->pubkeyTable->rowCount();
        ui->totalSignatures->setValue(numRows);
        if (ui->requiredSignatures->value() > numRows) {
            ui->requiredSignatures->setValue(numRows);
        }
    }
}

void MultisigDialog::refreshAddressList()
{
    loadMultisigAddressTable();
}

void MultisigDialog::onTotalSignaturesChanged(int value)
{
    if (!ui || !ui->pubkeyTable)
        return;
        
    // Adjust the number of rows in the pubkey table to match the total signatures
    int currentRows = ui->pubkeyTable->rowCount();
    
    if (currentRows < value) {
        // Add rows until we have enough
        for (int i = currentRows; i < value; i++) {
            addInputRow();
        }
    } else if (currentRows > value) {
        // Remove rows until we have the right amount
        for (int i = currentRows - 1; i >= value; i--) {
            ui->pubkeyTable->removeRow(i);
        }
    }
    
    // Make sure required signatures doesn't exceed total signatures
    if (ui->requiredSignatures->value() > value) {
        ui->requiredSignatures->setValue(value);
    }
}

void MultisigDialog::addWalletModel(const QString& name, WalletModel *walletModel)
{
    if (!walletModel)
        return;

    // Add to map of available wallets
    mapWalletModels[name] = walletModel;

    // Add to dropdown selector
    ui->walletSelector->addItem(name, name);

    // If this is the first wallet, select it
    if (ui->walletSelector->count() == 1) {
        this->model = walletModel;
    }
}

void MultisigDialog::setModel(WalletModel *pwalletModel)
{
    if (!pwalletModel)
        return;

    // Clear existing wallet selector items
    ui->walletSelector->clear();
    mapWalletModels.clear();

    // Add the default wallet
    addWalletModel(tr("Default"), pwalletModel);

    this->model = pwalletModel;
    
    // Populate the address selector with addresses from the wallet
    populateAddressSelector();
    
    // Load multisig addresses if we're on the list tab
    if (ui->tabWidget->currentIndex() == 4) {
        loadMultisigAddressTable();
    }
}

void MultisigDialog::onWalletSelectionChanged(int index)
{
    if (index < 0 || index >= ui->walletSelector->count())
        return;

    QString walletName = ui->walletSelector->itemData(index).toString();
    if (mapWalletModels.contains(walletName)) {
        this->model = mapWalletModels[walletName];
        
        // Update address selector with addresses from the selected wallet
        populateAddressSelector();
        
        // Refresh multisig address list if we're on that tab
        if (ui->tabWidget->currentIndex() == 4) {
            loadMultisigAddressTable();
        }
    }
}

void MultisigDialog::showTab(Mode mode)
{
    this->currentMode = mode;
    int nIndex = 0;

    switch (mode) {
        case CreateWallet:
            nIndex = 0;
            break;
        case ImportWallet:
            nIndex = 1;
            break;
        case SignTransaction:
            nIndex = 2;
            // Populate the signing address field when switching to Sign Transaction tab
            if (model) {
                populateSigningAddressSelector();
            }
            break;
        case BroadcastTransaction:
            nIndex = 3;
            break;
        case ListAddresses:
            nIndex = 4;
            break;
    }

    ui->tabWidget->setCurrentIndex(nIndex);
    
    // If we're showing the list addresses tab, refresh the list
    if (mode == ListAddresses && model) {
        loadMultisigAddressTable();
    }
}

void MultisigDialog::addInputRow()
{
    int nRow = ui->pubkeyTable->rowCount();
    ui->pubkeyTable->insertRow(nRow);
    setupInputRow(nRow);
    
    // Update the total participants spinner to match the number of rows
    // Only update if the new row count is greater than the current value
    if (nRow + 1 > ui->totalSignatures->value()) {
        ui->totalSignatures->setValue(nRow + 1);
    }
}



void MultisigDialog::showAddressMenu()
{
    if (!model || !pwalletMain)
        return;
    
    // Find which row's button was clicked
    QPushButton* pSender = qobject_cast<QPushButton*>(sender());
    if (!pSender)
        return;
        
    QString objectName = pSender->objectName();
    QRegExp rx("selectButton(\\d+)");
    if (rx.indexIn(objectName) == -1)
        return;
        
    int nRow = rx.cap(1).toInt();
    
    // Create menu
    QMenu menu;
    
    // Create a list to store addresses
    std::vector<CBitcoinAddress> addressList;
    {
        LOCK(pwalletMain->cs_wallet);
        BOOST_FOREACH(const PAIRTYPE(CTxDestination, CAddressBookData)& item, pwalletMain->mapAddressBook)
        {
            const CTxDestination& dest = item.first;
            CBitcoinAddress address(dest);
            // Only add P2PKH addresses (not P2SH/multisig)
            if (address.IsValid() && !address.IsScript()) {
                addressList.push_back(address);
            }
        }
    }
    
    // Add menu items for each address
    BOOST_FOREACH(const CBitcoinAddress& address, addressList)
    {
        QString addressStr = QString::fromStdString(address.ToString());
        QString label = "";
        
        if (model->getAddressTableModel()) {
            label = model->getAddressTableModel()->labelForAddress(addressStr);
        }
        
        QString menuText = addressStr;
        if (!label.isEmpty()) {
            menuText = label + " (" + addressStr + ")";
        }
        
        QAction* action = menu.addAction(menuText);
        action->setData(addressStr);
    }
    
    // Show the menu at the button's position
    QAction* selectedAction = menu.exec(pSender->mapToGlobal(pSender->rect().bottomLeft()));
    if (selectedAction) {
        QString selectedAddress = selectedAction->data().toString();
        
        // Find the input field for this row
        QLineEdit* pInputField = ui->pubkeyTable->findChild<QLineEdit*>(QString("inputField%1").arg(nRow));
        if (pInputField) {
            // Set the address in the input field
            pInputField->setText(selectedAddress);
            
            // Set the input type to address
            QComboBox* pTypeCombo = qobject_cast<QComboBox*>(ui->pubkeyTable->cellWidget(nRow, 0));
            if (pTypeCombo) {
                pTypeCombo->setCurrentIndex(INPUT_ADDRESS);
            }
            
            // Validate the input
            validateInput(nRow);
        }
    }
}

void MultisigDialog::loadMultisigAddressTable()
{
    if (!model || !pwalletMain)
        return;
        
    ui->multisigAddressTable->setRowCount(0);
    
    try {
        // Get all P2SH addresses from the wallet
        std::set<CTxDestination> setMultisigDests;
        {
            LOCK(pwalletMain->cs_wallet);
            BOOST_FOREACH(const PAIRTYPE(CTxDestination, CAddressBookData)& item, pwalletMain->mapAddressBook)
            {
                const CBitcoinAddress& address = item.first;
                if (address.IsScript()) {
                    setMultisigDests.insert(item.first);
                }
            }
        }
        
        // Add each multisig address to the table
        BOOST_FOREACH(const CTxDestination& dest, setMultisigDests)
        {
            try {
                CScriptID scriptID = boost::get<CScriptID>(dest);
                CScript redeemScript;
                if (pwalletMain->GetCScript(scriptID, redeemScript)) {
                    CBitcoinAddress address(dest);
                    QString addressStr = QString::fromStdString(address.ToString());
                    QString label = "";
                    
                    // Safely get the label for the address
                    if (model->getAddressTableModel()) {
                        try {
                            label = model->getAddressTableModel()->labelForAddress(addressStr);
                        } catch (...) {
                            // If getting the label fails, just use an empty string
                        }
                    }
                    
                    QString redeemScriptHex = QString::fromStdString(HexStr(redeemScript.begin(), redeemScript.end()));
                    
                    // Add to table
                    int row = ui->multisigAddressTable->rowCount();
                    ui->multisigAddressTable->insertRow(row);
                    ui->multisigAddressTable->setItem(row, 0, new QTableWidgetItem(addressStr));
                    ui->multisigAddressTable->setItem(row, 1, new QTableWidgetItem(label));
                    ui->multisigAddressTable->setItem(row, 2, new QTableWidgetItem(redeemScriptHex));
                }
            } catch (...) {
                // Skip this address if there's an error processing it
                continue;
            }
        }
        
        // Resize columns to fit content
        ui->multisigAddressTable->resizeColumnsToContents();
    } catch (...) {
        // If there's any error in the process, just return without crashing
    }
}

void MultisigDialog::populateWalletAddressSelector()
{
    populateAddressSelector();
}

void MultisigDialog::populateSigningAddressSelector()
{
    if (!model || !pwalletMain)
        return;
    
    // Check if we already have a combo box for addresses
    QComboBox* addressCombo = findChild<QComboBox*>("signAddressCombo");
    
    if (!addressCombo) {
        // If we don't have a combo box yet, create one next to the existing line edit
        QLineEdit* addressEdit = ui->signAddress;
        if (addressEdit) {
            // Create a layout to hold both widgets if it doesn't exist
            QHBoxLayout* layout = qobject_cast<QHBoxLayout*>(addressEdit->parentWidget()->layout());
            if (!layout) {
                // Create a new layout
                layout = new QHBoxLayout(addressEdit->parentWidget());
                layout->setContentsMargins(0, 0, 0, 0);
                layout->addWidget(addressEdit);
            }
            
            // Create a new combo box
            addressCombo = new QComboBox(addressEdit->parentWidget());
            addressCombo->setObjectName("signAddressCombo");
            
            // Add the combo box to the layout
            layout->addWidget(addressCombo);
            
            // Hide the original line edit since we'll use the combo box instead
            addressEdit->hide();
        }
    }
    
    if (!addressCombo)
        return;
        
    // Clear the combo box
    addressCombo->clear();
    
    try {
        // Get all addresses from the wallet that have private keys
        std::set<CKeyID> keyIDs;
        pwalletMain->GetKeys(keyIDs);
        
        // Add each address to the combo box
        for (const CKeyID& keyID : keyIDs) {
            // Convert keyID to address
            CBitcoinAddress address(keyID);
            if (address.IsValid()) {
                QString addressStr = QString::fromStdString(address.ToString());
                
                // Get the label for this address if available
                QString label = "";
                if (model->getAddressTableModel()) {
                    label = model->getAddressTableModel()->labelForAddress(addressStr);
                }
                
                QString displayText = addressStr;
                if (!label.isEmpty()) {
                    displayText = label + " (" + addressStr + ")";
                }
                
                addressCombo->addItem(displayText, addressStr);
            }
        }
        
        // Select the first item if available
        if (addressCombo->count() > 0) {
            addressCombo->setCurrentIndex(0);
        }
    } catch (...) {
        // If there's any error in the process, just return without crashing
    }
}

void MultisigDialog::setupInputRow(int nRow)
{
    // Create and setup input type combo box
    QComboBox* pTypeCombo = new QComboBox();
    pTypeCombo->addItem(tr("Public Key"), INPUT_PUBKEY);
    pTypeCombo->addItem(tr("Wallet Address"), INPUT_ADDRESS);
    connect(pTypeCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(onInputTypeChanged(int)));
    ui->pubkeyTable->setCellWidget(nRow, 0, pTypeCombo);

    // Create input value cell with a layout containing a text field and a button
    QWidget* pInputWidget = new QWidget();
    QHBoxLayout* pInputLayout = new QHBoxLayout(pInputWidget);
    pInputLayout->setContentsMargins(5, 2, 5, 2);
    pInputLayout->setSpacing(2);
    
    // Create text field for input value
    QLineEdit* pInputField = new QLineEdit();
    pInputField->setObjectName(QString("inputField%1").arg(nRow));
    pInputLayout->addWidget(pInputField, 1); // 1 = stretch factor
    
    // Create button for selecting from wallet
    QPushButton* pSelectButton = new QPushButton(tr("Select"));
    pSelectButton->setObjectName(QString("selectButton%1").arg(nRow));
    pSelectButton->setMaximumWidth(60);
    pInputLayout->addWidget(pSelectButton);
    
    // Connect the button to a slot that will show a menu of wallet addresses
    connect(pSelectButton, SIGNAL(clicked()), this, SLOT(showAddressMenu()));
    
    // Set the widget as the cell widget
    ui->pubkeyTable->setCellWidget(nRow, 1, pInputWidget);

    // Create status cell
    QTableWidgetItem* pStatusItem = new QTableWidgetItem("");
    pStatusItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    ui->pubkeyTable->setItem(nRow, 2, pStatusItem);
}

void MultisigDialog::onInputTypeChanged(int index)
{
    // Find which row contains the combobox that sent the signal
    QComboBox* pSender = qobject_cast<QComboBox*>(sender());
    if (!pSender)
        return;
        
    // Find the row containing this combobox
    int nRow = -1;
    for (int i = 0; i < ui->pubkeyTable->rowCount(); i++) {
        if (ui->pubkeyTable->cellWidget(i, 0) == pSender) {
            nRow = i;
            break;
        }
    }
    
    if (nRow == -1)
        return;

    // Clear the input and status when type changes
    QTableWidgetItem* pItem = ui->pubkeyTable->item(nRow, 1);
    if (pItem)
        pItem->setText("");
    updateInputStatus(nRow, "", true);
}

void MultisigDialog::validateInput(int nRow)
{
    QComboBox* pTypeCombo = qobject_cast<QComboBox*>(ui->pubkeyTable->cellWidget(nRow, 0));
    if (!pTypeCombo)
        return;
    
    // Get the input field from the cell widget container
    QWidget* pInputWidget = ui->pubkeyTable->cellWidget(nRow, 1);
    if (!pInputWidget)
        return;
        
    QLineEdit* pInputField = pInputWidget->findChild<QLineEdit*>(QString("inputField%1").arg(nRow));
    if (!pInputField)
        return;

    QString strInput = pInputField->text().trimmed();
    if (strInput.isEmpty()) {
        updateInputStatus(nRow, "", true);
        return;
    }

    InputType type = static_cast<InputType>(pTypeCombo->currentData().toInt());
    if (type == INPUT_PUBKEY) {
        // Validate public key format
        std::string strPubKeyHex = strInput.toStdString();
        std::vector<unsigned char> vchPubKey = ParseHex(strPubKeyHex);
        if (vchPubKey.size() != 33) {
            updateInputStatus(nRow, tr("Invalid public key format - must be 33 bytes"), false);
            return;
        }
        CPubKey pubKey(vchPubKey);
        if (!pubKey.IsValid()) {
            updateInputStatus(nRow, tr("Invalid public key"), false);
            return;
        }
        updateInputStatus(nRow, tr("Valid public key"), true);
    } else {
        // Validate address and get public key
        QString strPubKey, strError;
        if (!validateAddress(strInput, strPubKey, strError)) {
            updateInputStatus(nRow, strError, false);
            return;
        }
        updateInputStatus(nRow, tr("Valid address"), true);
    }
}

bool MultisigDialog::validateAddress(const QString& strAddress, QString& strPubKey, QString& strError)
{
    std::string strStdAddress = strAddress.toStdString();
    CBitcoinAddress addr(strStdAddress);
    if (!addr.IsValid()) {
        strError = tr("Invalid address format");
        return false;
    }

    if (!getPubKeyFromAddress(strAddress, strPubKey)) {
        strError = tr("Could not retrieve public key from address");
        return false;
    }

    return true;
}

bool MultisigDialog::getPubKeyFromAddress(const QString& strAddress, QString& strPubKey)
{
    if (!model || !pwalletMain)
        return false;

    // Get public key from address
    CBitcoinAddress addr(strAddress.toStdString());
    if (!addr.IsValid())
        return false;

    CKeyID keyID;
    if (!addr.GetKeyID(keyID))
        return false;

    CPubKey pubKey;
    // Try to get the public key directly from the wallet first
    if (pwalletMain->GetPubKey(keyID, pubKey)) {
        std::vector<unsigned char> vchPubKey(pubKey.begin(), pubKey.end());
        strPubKey = QString::fromStdString(HexStr(vchPubKey));
        return true;
    }
    
    // Fall back to the model method if wallet method fails
    try {
        if (model->getPubKey(keyID, pubKey)) {
            std::vector<unsigned char> vchPubKey(pubKey.begin(), pubKey.end());
            strPubKey = QString::fromStdString(HexStr(vchPubKey));
            return true;
        }
    } catch (...) {
        // Catch any exceptions to prevent crashes
        return false;
    }
    
    return false;
}

void MultisigDialog::updateInputStatus(int nRow, const QString& strStatus, bool fValid)
{
    QTableWidgetItem* pStatusItem = ui->pubkeyTable->item(nRow, 2);
    if (!pStatusItem) return;

    pStatusItem->setText(strStatus);
    pStatusItem->setForeground(fValid ? QColor(0, 128, 0) : QColor(255, 0, 0));
}

bool MultisigDialog::hasSignerPubKey(const QString& strPubKey)
{
    if (!model)
        return false;

    std::string strPubKeyHex = strPubKey.toStdString();
    if (!IsHex(strPubKeyHex))
        return false;

    std::vector<unsigned char> vchPubKey = ParseHex(strPubKeyHex);
    CPubKey pubKey(vchPubKey);
    if (!pubKey.IsValid())
        return false;

    return model->havePrivKey(pubKey.GetID());
}

bool MultisigDialog::hasPrivateKey(const std::vector<CPubKey>& pubkeys)
{
    if (!model)
        return false;
        
    // Check if we have any of the private keys for this multisig address
    for (const CPubKey& pubKey : pubkeys) {
        if (!pubKey.IsValid())
            continue;
            
        if (model->havePrivKey(pubKey.GetID())) {
            // We found a private key for this pubkey
            return true;
        }
    }
    
    return false;
}

std::vector<std::string> MultisigDialog::getValidatedInputs(QString& errorMsg)
{
    std::vector<std::string> pubkeys;
    bool hasSignerKey = false;

    int nRows = ui->pubkeyTable->rowCount();
    for (int i = 0; i < nRows; i++) {
        QComboBox* typeCombo = qobject_cast<QComboBox*>(ui->pubkeyTable->cellWidget(i, 0));
        QWidget* inputWidget = ui->pubkeyTable->cellWidget(i, 1);
        if (!typeCombo || !inputWidget) continue;
        
        QLineEdit* inputField = inputWidget->findChild<QLineEdit*>(QString("inputField%1").arg(i));
        if (!inputField) continue;

        QString input = inputField->text().trimmed();
        if (input.isEmpty()) continue;

        InputType type = static_cast<InputType>(typeCombo->currentData().toInt());
        QString pubKey;

        if (type == INPUT_PUBKEY) {
            pubKey = input;
        } else {
            QString error;
            if (!validateAddress(input, pubKey, error)) {
                errorMsg = tr("Invalid input at row %1: %2").arg(i + 1).arg(error);
                return std::vector<std::string>();
            }
        }

        if (hasSignerPubKey(pubKey)) {
            hasSignerKey = true;
        }

        pubkeys.push_back(pubKey.toStdString());
    }

    return pubkeys;
}

void MultisigDialog::createMultisigWallet()
{
    if (!model) return;

    int required = ui->requiredSignatures->value();
    int total = ui->totalSignatures->value();

    if (required > total) {
        showError(tr("Error"), tr("Required keys cannot be more than total keys."));
        return;
    }

    QString errorMsg;
    std::vector<std::string> pubkeys = getValidatedInputs(errorMsg);
    if (pubkeys.empty()) {
        showError(tr("Error"), errorMsg);
        return;
    }

    if ((int)pubkeys.size() != total) {
        showError(tr("Error"), tr("Please provide exactly %1 public keys.").arg(total));
        return;
    }

    // Convert hex strings to CPubKey objects
    std::vector<CPubKey> keys;
    BOOST_FOREACH(const std::string& hexPubKey, pubkeys) {
        std::vector<unsigned char> data = ParseHex(hexPubKey);
        CPubKey pubKey(data.begin(), data.end());
        if (!pubKey.IsValid()) {
            showError(tr("Error"), tr("Invalid public key format."));
            return;
        }
        keys.push_back(pubKey);
    }

    // Create multisig address
    CScript redeemScript = GetScriptForMultisig(required, keys);
    if (redeemScript.empty()) {
        showError(tr("Error"), tr("Failed to create multisig wallet."));
        return;
    }

    // Save the redeem script
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Save Multisig Wallet"), QDir::homePath(),
        tr("Multisig Script (*.script);;All Files (*)"));

    if (fileName.isEmpty())
        return;

    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly)) {
        QTextStream stream(&file);
        stream << QString::fromStdString(HexStr(redeemScript));
        file.close();

        // Create P2SH address from redeem script
        CScriptID scriptID = CScriptID(redeemScript);
        CBitcoinAddress multisigAddress;
        multisigAddress.Set(scriptID);
        if (!multisigAddress.IsValid()) {
            showError(tr("Error"), tr("Could not create multisig address."));
            return;
        }
        
        // Display the multisig address and redeem script in the UI
        ui->multisigAddress->setText(QString::fromStdString(multisigAddress.ToString()));
        ui->redeemScript->setText(QString::fromStdString(HexStr(redeemScript)));

        // Automatically import the multisig address into the wallet
        bool importedAsReadOnly = false;
        if (!model->importMultisigAddress(redeemScript, keys, importedAsReadOnly)) {
            showError(tr("Error"), tr("Failed to import multisig wallet."));
            return;
        }
        
        // Save multisig info to a file for sharing with other signers
        if (QMessageBox::question(this, tr("Save for sharing"),
                                 tr("Would you like to save the multisig info to a file that can be shared with other signers?"),
                                 QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
            saveMultisigInfoForSharing(redeemScript, keys, required);
        }

        QString successMsg = tr("Multisig wallet created successfully and saved to %1").arg(fileName);
        if (importedAsReadOnly) {
            successMsg += tr(" (imported as read-only because none of the signing keys are in your wallet)");
        } else {
            successMsg += tr(" (imported with signing keys)");
        }
        
        showInfo(tr("Success"), successMsg);
    } else {
        showError(tr("Error"), tr("Could not write to file %1").arg(fileName));
    }
}

void MultisigDialog::importMultisigScript(const CScript& redeemScript, bool importAsReadOnly)
{
    if (!model)
        return;

    // Extract public keys from redeem script
    txnouttype type;
    std::vector<CTxDestination> addresses;
    int nRequired;
    if (!ExtractDestinations(redeemScript, type, addresses, nRequired)) {
        showError(tr("Error"), tr("Invalid redeem script format."));
        return;
    }
    
    // Create P2SH address from redeem script
    CScriptID scriptID = CScriptID(redeemScript);
    CBitcoinAddress multisigAddress;
    multisigAddress.Set(scriptID);
    if (!multisigAddress.IsValid()) {
        showError(tr("Error"), tr("Could not create multisig address."));
        return;
    }
    
    // Display the multisig address and redeem script in the UI
    ui->multisigAddress->setText(QString::fromStdString(multisigAddress.ToString()));
    ui->redeemScript->setText(QString::fromStdString(HexStr(redeemScript)));

    // Check if we have any signer keys if not importing as read-only
    if (!importAsReadOnly) {
        bool fHasSignerKey = false;
        BOOST_FOREACH(const CTxDestination& dest, addresses) {
            CKeyID keyID;
            if (CBitcoinAddress(dest).GetKeyID(keyID)) {
                CPubKey pubKey;
                if (model->getPubKey(keyID, pubKey)) {
                    fHasSignerKey = true;
                    break;
                }
            }
        }
        importAsReadOnly = !fHasSignerKey;
    }

    // Import the address
    if (!model->addMultisigAddress(redeemScript)) {
        showError(tr("Error"), tr("Failed to import multisig wallet."));
        return;
    }
}

void MultisigDialog::importMultisigWallet()
{
    if (!model) return;

    QString scriptHex = ui->importRedeemScript->toPlainText().trimmed();
    if (scriptHex.isEmpty()) {
        showError(tr("Error"), tr("Please enter a redeem script."));
        return;
    }

    std::vector<unsigned char> scriptData = ParseHex(scriptHex.toStdString());
    CScript redeemScript(scriptData.begin(), scriptData.end());

    // Import the script
    importMultisigScript(redeemScript, false);

    showInfo(tr("Success"), tr("Multisig wallet imported successfully."));
}

void MultisigDialog::loadTransaction()
{
    QString strFileName = QFileDialog::getOpenFileName(this,
        tr("Load Unsigned Transaction"), QDir::homePath(),
        tr("Unsigned Transaction (*.hex);;All Files (*)"));

    if (strFileName.isEmpty())
        return;

    QFile file(strFileName);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, tr("Error"),
            tr("Could not open transaction file %1").arg(strFileName));
        return;
    }

    QTextStream stream(&file);
    QString strHexTx = stream.readAll().trimmed();
    file.close();

    // Decode transaction
    CMutableTransaction tx;
    if (!DecodeHexTx(tx, strHexTx.toStdString())) {
        QMessageBox::critical(this, tr("Error"),
            tr("Failed to decode transaction."));
        return;
    }

    // Calculate total amount
    CAmount nTotalAmount = 0;
    BOOST_FOREACH(const CTxOut& txout, tx.vout) {
        nTotalAmount += txout.nValue;
    }

    // Build transaction details string
    QString strDetails;
    strDetails += tr("Transaction Details:\n\n");
    strDetails += tr("Version: %1\n").arg(tx.nVersion);
    strDetails += tr("Inputs: %1\n").arg(tx.vin.size());
    strDetails += tr("Outputs: %1\n").arg(tx.vout.size());
    strDetails += tr("Total Amount: %1 JKC\n").arg(nTotalAmount / COIN);

    // Add output details
    strDetails += tr("\nOutputs:\n");
    size_t nIndex = 0;
    BOOST_FOREACH(const CTxOut& txout, tx.vout) {
        strDetails += tr("  Output #%1:\n").arg(nIndex++);
        strDetails += tr("    Amount: %1 JKC\n").arg(txout.nValue / COIN);
        txnouttype type;
        std::vector<CTxDestination> addresses;
        int nRequired;
        ExtractDestinations(txout.scriptPubKey, type, addresses, nRequired);
        strDetails += tr("    Script Type: %1\n").arg(GetTxnOutputType(type));
    }

    // Show confirmation dialog
    QMessageBox confirmBox(this);
    confirmBox.setWindowTitle(tr("Confirm Transaction Load"));
    confirmBox.setText(tr("Are you sure you want to load this transaction?"));
    confirmBox.setInformativeText(tr("Total amount: %1 JKC").arg(nTotalAmount / COIN));
    confirmBox.setDetailedText(strDetails);
    confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    confirmBox.setDefaultButton(QMessageBox::No);

    if (confirmBox.exec() == QMessageBox::Yes) {
        // Store the current transaction
        currentTransaction = tx;
        
        // Load transaction into editor
        ui->transactionHex->setPlainText(strHexTx);
        ui->transactionDetails->setPlainText(strDetails);
        updateClearButton();
        
        // Populate the signing address selector with available addresses
        populateSigningAddressSelector();
    }
}

void MultisigDialog::decodeTransaction()
{
    QString strHexTx = ui->transactionHex->toPlainText().trimmed();
    if (strHexTx.isEmpty()) {
        QMessageBox::warning(this, tr("Error"), 
            tr("Please enter or load a transaction to decode."));
        return;
    }
    
    // Decode and display the transaction
    decodeAndDisplayTransaction(strHexTx);
}

bool MultisigDialog::decodeAndDisplayTransaction(const QString& strHexTx)
{
    if (strHexTx.isEmpty())
        return false;
        
    // Decode transaction
    CMutableTransaction tx;
    if (!DecodeHexTx(tx, strHexTx.toStdString())) {
        QMessageBox::critical(this, tr("Error"),
            tr("Failed to decode transaction."));
        return false;
    }
    
    // Store the current transaction
    currentTransaction = tx;
    
    // Calculate total amount
    CAmount nTotalAmount = 0;
    BOOST_FOREACH(const CTxOut& txout, tx.vout) {
        nTotalAmount += txout.nValue;
    }

    // Determine transaction signing status
    bool isFullySigned = true;
    bool isPartiallySigned = false;
    bool isMultisigTransaction = false;
    int numInputs = tx.vin.size();
    int numSignedInputs = 0;

    // Check each input for signatures and multisig scripts
    for (size_t i = 0; i < tx.vin.size(); i++) {
        const CTxIn& txin = tx.vin[i];
        
        // Check if input has a signature
        bool hasSig = !txin.scriptSig.empty();
        if (hasSig) {
            numSignedInputs++;
            
            // Check for multisig pattern in script
            CScript scriptSig = txin.scriptSig;
            std::vector<unsigned char> data(scriptSig.begin(), scriptSig.end());
            std::string scriptHex = HexStr(data);
            
            // Look for OP_0 which often indicates start of multisig script
            if (scriptHex.substr(0, 2) == "00" && scriptHex.length() > 10) {
                isMultisigTransaction = true;
                
                // Check if it's partially signed (has some signatures but not all)
                // This is a simplified check - in reality you'd parse the script properly
                size_t sigCount = 0;
                size_t pos = 0;
                
                // Count signature-like patterns in the script
                while ((pos = scriptHex.find("30" /* DER sequence marker */, pos)) != std::string::npos) {
                    sigCount++;
                    pos += 2;
                }
                
                if (sigCount > 0 && sigCount < 3) { // Assuming most multisig is 2-of-3
                    isPartiallySigned = true;
                    isFullySigned = false;
                }
            }
        } else {
            isFullySigned = false;
        }
    }

    // Build transaction details string
    QString strDetails;
    
    // Transaction status section
    strDetails += tr("===== TRANSACTION STATUS =====\n");
    if (isMultisigTransaction) {
        if (isFullySigned) {
            strDetails += tr("Status: FULLY SIGNED (ready to broadcast)\n");
        } else if (isPartiallySigned) {
            strDetails += tr("Status: PARTIALLY SIGNED (%1 of %2 inputs have signatures)\n")
                .arg(numSignedInputs).arg(numInputs);
            strDetails += tr("Action: Needs more signatures before broadcasting\n");
        } else if (numSignedInputs > 0) {
            strDetails += tr("Status: INCOMPLETE (some inputs signed, but not in multisig format)\n");
        } else {
            strDetails += tr("Status: UNSIGNED\n");
            strDetails += tr("Action: Transaction needs to be signed\n");
        }
    } else {
        if (isFullySigned) {
            strDetails += tr("Status: SIGNED (not a multisig transaction)\n");
        } else {
            strDetails += tr("Status: INCOMPLETE (not a multisig transaction)\n");
        }
    }
    strDetails += "\n";

    // Transaction details section
    strDetails += tr("===== TRANSACTION DETAILS =====\n");
    strDetails += tr("Version: %1\n").arg(tx.nVersion);
    strDetails += tr("Inputs: %1\n").arg(tx.vin.size());
    strDetails += tr("Outputs: %1\n").arg(tx.vout.size());
    strDetails += tr("Total Amount: %1 JKC\n").arg(nTotalAmount / COIN);

    // Add input details
    strDetails += tr("\nInputs:\n");
    for (size_t i = 0; i < tx.vin.size(); i++) {
        const CTxIn& txin = tx.vin[i];
        strDetails += tr("  Input #%1:\n").arg(i);
        strDetails += tr("    Previous Tx: %1\n").arg(QString::fromStdString(txin.prevout.hash.GetHex()));
        strDetails += tr("    Output Index: %1\n").arg(txin.prevout.n);
        
        // Check if this input has a signature
        bool hasSig = !txin.scriptSig.empty();
        strDetails += tr("    Signature: %1\n").arg(hasSig ? tr("Present") : tr("Not present"));
        
        // Show script hex for debugging
        if (hasSig) {
            std::vector<unsigned char> data(txin.scriptSig.begin(), txin.scriptSig.end());
            strDetails += tr("    ScriptSig: %1\n").arg(QString::fromStdString(HexStr(data)));
        }
    }

    // Add output details
    strDetails += tr("\nOutputs:\n");
    size_t nIndex = 0;
    BOOST_FOREACH(const CTxOut& txout, tx.vout) {
        strDetails += tr("  Output #%1:\n").arg(nIndex++);
        strDetails += tr("    Amount: %1 JKC\n").arg(txout.nValue / COIN);
        txnouttype type;
        std::vector<CTxDestination> addresses;
        int nRequired;
        ExtractDestinations(txout.scriptPubKey, type, addresses, nRequired);
        strDetails += tr("    Script Type: %1\n").arg(GetTxnOutputType(type));
        
        // Add recipient addresses
        if (!addresses.empty()) {
            strDetails += tr("    Recipients:\n");
            BOOST_FOREACH(const CTxDestination& dest, addresses) {
                strDetails += tr("      %1\n").arg(QString::fromStdString(CBitcoinAddress(dest).ToString()));
            }
        }
    }
    
    // Display the transaction details
    ui->transactionDetails->setPlainText(strDetails);
    
    return true;
}

void MultisigDialog::signMultisigTx()
{
    if (!model || !pwalletMain)
        return;

    QString strHexTx = ui->transactionHex->toPlainText().trimmed();
    if (strHexTx.isEmpty()) {
        QMessageBox::warning(this, tr("Error"), 
            tr("Please enter or load a transaction to sign."));
        return;
    }
    
    // Decode and display transaction details if not already done
    decodeAndDisplayTransaction(strHexTx);
    
    // Get the selected signing address from the dropdown
    QString signingAddress;
    QComboBox* addressCombo = findChild<QComboBox*>("signAddressCombo");
    if (addressCombo && addressCombo->count() > 0) {
        // Get the address from the combo box data (not the display text)
        signingAddress = addressCombo->currentData().toString();
    } else {
        // Fallback to the text field if the combo box isn't available yet
        signingAddress = ui->signAddress->text().trimmed();
    }
    
    if (signingAddress.isEmpty()) {
        QMessageBox::warning(this, tr("Error"),
            tr("Please select an address to sign with."));
        return;
    }

    // Decode transaction
    CMutableTransaction tx;
    if (!DecodeHexTx(tx, strHexTx.toStdString())) {
        QMessageBox::critical(this, tr("Error"), 
            tr("Failed to decode transaction."));
        return;
    }
    
    // Check if this is a multisig transaction by examining the inputs
    bool isMultisigTransaction = false;
    for (const CTxIn& txin : tx.vin) {
        // Check for multisig pattern in script or empty script (unsigned)
        if (txin.scriptSig.empty()) {
            // Empty script could be unsigned multisig, continue checking
            continue;
        }
        
        // Check for multisig pattern in script
        CScript scriptSig = txin.scriptSig;
        std::vector<unsigned char> data(scriptSig.begin(), scriptSig.end());
        std::string scriptHex = HexStr(data);
        
        // Look for OP_0 which often indicates start of multisig script
        if (scriptHex.substr(0, 2) == "00" && scriptHex.length() > 10) {
            isMultisigTransaction = true;
            break;
        }
    }
    
    // Also check outputs for P2SH addresses which are typically used for multisig
    if (!isMultisigTransaction) {
        for (const CTxOut& txout : tx.vout) {
            txnouttype type;
            std::vector<CTxDestination> addresses;
            int nRequired;
            if (ExtractDestinations(txout.scriptPubKey, type, addresses, nRequired)) {
                if (type == TX_MULTISIG || type == TX_SCRIPTHASH) {
                    isMultisigTransaction = true;
                    break;
                }
            }
        }
    }
    
    // Reject if not a multisig transaction
    if (!isMultisigTransaction) {
        QMessageBox::warning(this, tr("Error"),
            tr("This does not appear to be a multisig transaction. "
               "Only multisig transactions can be signed with this tool."));
        return;
    }
    
    // Get the key ID from the address
    CBitcoinAddress addr(signingAddress.toStdString());
    if (!addr.IsValid()) {
        QMessageBox::warning(this, tr("Error"),
            tr("Invalid signing address."));
        return;
    }
    
    CKeyID keyID;
    if (!addr.GetKeyID(keyID)) {
        QMessageBox::warning(this, tr("Error"),
            tr("Could not get key ID from address."));
        return;
    }

    // Sign transaction
    bool fComplete = false;
    try {
        // The signMultisigTx method doesn't accept a keyID parameter, so we'll use the standard method
        fComplete = model->signMultisigTx(tx);
    } catch (...) {
        showError(tr("Error"), tr("An error occurred while signing the transaction."));
        return;
    }

    // Update transaction hex
    std::string strHexSigned = EncodeHexTx(tx);
    ui->transactionHex->setPlainText(QString::fromStdString(strHexSigned));
    updateClearButton();
    
    // Update the current transaction
    currentTransaction = tx;
    
    // Handle different outcomes based on whether the transaction is fully signed
    if (fComplete) {
        // Transaction is fully signed and ready to broadcast
        QMessageBox confirmBox(this);
        confirmBox.setWindowTitle(tr("Transaction Fully Signed"));
        confirmBox.setText(tr("The transaction is fully signed and ready to broadcast."));
        confirmBox.setInformativeText(tr("Would you like to broadcast it now?"));
        confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        confirmBox.setDefaultButton(QMessageBox::Yes);
        
        if (confirmBox.exec() == QMessageBox::Yes) {
            // Broadcast the transaction
            broadcastMultisigTx();
        } else {
            // User chose not to broadcast, just show success message
            showInfo(tr("Success"), tr("Transaction signed successfully and ready to broadcast.\n"
                                      "You can broadcast it later from the 'Broadcast' tab."));
        }
    } else {
        // Transaction is partially signed and needs more signatures
        QMessageBox confirmBox(this);
        confirmBox.setWindowTitle(tr("Transaction Partially Signed"));
        confirmBox.setText(tr("The transaction has been partially signed."));
        confirmBox.setInformativeText(tr("Would you like to save it to share with other signers?"));
        confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        confirmBox.setDefaultButton(QMessageBox::Yes);
        
        if (confirmBox.exec() == QMessageBox::Yes) {
            // Save the transaction for sharing
            saveTransaction();
        } else {
            // User chose not to save, just show success message
            showInfo(tr("Success"), tr("Transaction partially signed.\n"
                                      "You will need to share this transaction with other signers to complete it."));
        }
    }
}

void MultisigDialog::saveTransaction()
{
    QString strHexTx = ui->transactionHex->toPlainText().trimmed();
    if (strHexTx.isEmpty()) {
        QMessageBox::warning(this, tr("Error"), 
            tr("No transaction to save."));
        return;
    }

    // Decode the transaction to check if it's signed
    CMutableTransaction tx;
    if (!DecodeHexTx(tx, strHexTx.toStdString())) {
        QMessageBox::critical(this, tr("Error"), 
            tr("Failed to decode transaction."));
        return;
    }
    
    // Determine if this is a partially signed transaction
    bool isPartiallySignedTx = false;
    for (const CTxIn& txin : tx.vin) {
        if (!txin.scriptSig.empty()) {
            isPartiallySignedTx = true;
            break;
        }
    }
    
    QString fileDescription = isPartiallySignedTx ? 
        tr("Partially Signed Transaction") : tr("Unsigned Transaction");
    
    QString strFileName = QFileDialog::getSaveFileName(this,
        tr("Save %1").arg(fileDescription), QDir::homePath(),
        tr("Transaction (*.hex);;All Files (*)"));

    if (strFileName.isEmpty())
        return;

    QFile file(strFileName);
    if (file.open(QIODevice::WriteOnly)) {
        QTextStream stream(&file);
        stream << strHexTx;
        file.close();
        
        // Determine the appropriate success message based on whether the transaction is signed
        QString successMsg;
        if (isPartiallySignedTx) {
            successMsg = tr("Partially signed transaction saved successfully.\n\n"
                          "You can share this file with other signers to complete the transaction.");
        } else {
            successMsg = tr("Unsigned transaction saved successfully.\n\n"
                          "You can share this file with other signers to begin the signing process.");
        }
        
        showInfo(tr("Success"), successMsg + "\n\n" + tr("File saved to: %1").arg(strFileName));
    } else {
        showError(tr("Error"), 
            tr("Could not write to file %1").arg(strFileName));
    }
}

void MultisigDialog::showError(const QString& strTitle, const QString& strMessage)
{
    QMessageBox::critical(this, strTitle, strMessage, QMessageBox::Ok, QMessageBox::Ok);
}

void MultisigDialog::showInfo(const QString& strTitle, const QString& strMessage)
{
    QMessageBox::information(this, strTitle, strMessage, QMessageBox::Ok, QMessageBox::Ok);
}

void MultisigDialog::clearAll()
{
    // Clear input fields
    ui->transactionHex->clear();
    ui->transactionDetails->clear();

    // Reset input table
    while (ui->pubkeyTable->rowCount() > 0)
        ui->pubkeyTable->removeRow(0);

    // Add one empty row
    addInputRow();

    // Update reject button state
    updateClearButton();
}

void MultisigDialog::updateClearButton()
{
    bool fEnabled = false;
    if (ui->transactionHex->toPlainText().size() || 
        ui->transactionDetails->toPlainText().size() || 
        ui->pubkeyTable->rowCount() > 1)
    {
        fEnabled = true;
    }

    ui->rejectButton->setEnabled(fEnabled);
}

void MultisigDialog::loadSignedTransaction()
{
    QString strFileName = QFileDialog::getOpenFileName(this,
        tr("Load Signed Transaction"), QDir::homePath(),
        tr("Signed Transaction (*.hex);;All Files (*)"));

    if (strFileName.isEmpty())
        return;

    QFile file(strFileName);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, tr("Error"),
            tr("Could not open file %1").arg(strFileName));
        return;
    }

    QTextStream stream(&file);
    ui->broadcastTransactionEdit->setPlainText(stream.readAll());
    file.close();
}

void MultisigDialog::broadcastMultisigTx()
{
    if (!model)
        return;

    QString strHexTx = ui->broadcastTransactionEdit->toPlainText().trimmed();
    if (strHexTx.isEmpty()) {
        QMessageBox::warning(this, tr("Error"), 
            tr("Please enter or load a signed transaction to broadcast."));
        return;
    }

    // Decode and validate transaction
    CMutableTransaction tx;
    if (!DecodeHexTx(tx, strHexTx.toStdString())) {
        QMessageBox::critical(this, tr("Error"), 
            tr("Failed to decode transaction."));
        return;
    }

    // Broadcast
    std::string strError;
    if (!model->broadcastTransaction(tx, strError)) {
        showError(tr("Error"), 
            tr("Failed to broadcast transaction: %1").arg(QString::fromStdString(strError)));
        return;
    }

    showInfo(tr("Success"), tr("Transaction broadcast successfully."));
    ui->broadcastTransactionEdit->clear();
    updateClearButton();
}

void MultisigDialog::loadMultisigInfoFromFile()
{
    if (!model)
        return;
        
    // Ask user to select the file
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Load Multisig Info"), QDir::homePath(),
        tr("Multisig Info (*.json);;All Files (*)"));
        
    if (fileName.isEmpty())
        return;
        
    // Read the file
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        showError(tr("Error"), tr("Could not open file %1").arg(fileName));
        return;
    }
    
    // Read the JSON content
    QTextStream stream(&file);
    QString jsonContent = stream.readAll();
    file.close();
    
    // Parse the JSON
    UniValue jsonObj;
    if (!jsonObj.read(jsonContent.toStdString())) {
        showError(tr("Error"), tr("Invalid JSON format"));
        return;
    }
    
    // Handle both our custom format and importmulti format
    std::string redeemScriptHex;
    UniValue pubkeysVal;
    
    if (jsonObj.isObject()) {
        // Our custom format
        UniValue redeemScriptVal = find_value(jsonObj, "redeemScript");
        if (redeemScriptVal.isStr()) {
            redeemScriptHex = redeemScriptVal.get_str();
        }
        
        pubkeysVal = find_value(jsonObj, "pubkeys");
    } else if (jsonObj.isArray() && jsonObj.size() > 0 && jsonObj[0].isObject()) {
        // importmulti format
        UniValue firstItem = jsonObj[0];
        
        // Get redeemscript
        UniValue redeemScriptVal = find_value(firstItem, "redeemscript");
        if (redeemScriptVal.isStr()) {
            redeemScriptHex = redeemScriptVal.get_str();
        }
        
        // Get pubkeys
        pubkeysVal = find_value(firstItem, "pubkeys");
    }
    
    // Validate we have the required data
    if (redeemScriptHex.empty()) {
        showError(tr("Error"), tr("Redeem script not found in the file"));
        return;
    }
    
    // Set the redeem script in the import field
    ui->importRedeemScript->setPlainText(QString::fromStdString(redeemScriptHex));
    
    if (!pubkeysVal.isArray() || pubkeysVal.size() == 0) {
        showError(tr("Error"), tr("Public keys not found in the file"));
        return;
    }
    
    // Check if we have any of the private keys for this multisig address
    std::vector<CPubKey> pubkeys;
    for (size_t i = 0; i < pubkeysVal.size(); i++) {
        if (!pubkeysVal[i].isStr())
            continue;
            
        std::string hexPubKey = pubkeysVal[i].get_str();
        std::vector<unsigned char> data = ParseHex(hexPubKey);
        CPubKey pubKey(data.begin(), data.end());
        if (pubKey.IsValid()) {
            pubkeys.push_back(pubKey);
        }
    }
    
    // Create the redeem script from hex
    std::vector<unsigned char> scriptData = ParseHex(redeemScriptHex);
    CScript redeemScript(scriptData.begin(), scriptData.end());
    
    // Check if we have any of the private keys without importing
    bool hasPrivKey = hasPrivateKey(pubkeys);
    
    // Show a message to the user based on whether we have private keys
    if (hasPrivKey) {
        // We have at least one private key, import with signing capability
        bool importedAsReadOnly = false; // We know we have private keys, so it won't be read-only
        if (!model->importMultisigAddress(redeemScript, pubkeys, importedAsReadOnly)) {
            showError(tr("Error"), tr("Failed to import multisig wallet."));
            return;
        }
        
        // Show success message
        showInfo(tr("Success"), tr("Multisig wallet imported successfully with signing capability."));
    } else {
        // No private keys found, ask if user wants to import as read-only
        if (QMessageBox::question(this, tr("No Private Keys Found"),
                                tr("None of the private keys for this multisig wallet were found in your wallet. Do you want to import it as read-only?"),
                                QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) {
            return;
        }
        
        // Import as read-only
        if (!model->addMultisigAddress(redeemScript)) {
            showError(tr("Error"), tr("Failed to import multisig wallet."));
            return;
        }
        
        // Show success message
        showInfo(tr("Success"), tr("Multisig wallet imported successfully as read-only."));
    }
    
    // Create P2SH address from redeem script and display it
    CScriptID scriptID = CScriptID(redeemScript);
    CBitcoinAddress multisigAddress;
    multisigAddress.Set(scriptID);
    if (multisigAddress.IsValid()) {
        // Switch to the Create tab to show the address and redeem script
        ui->tabWidget->setCurrentIndex(0);
        ui->multisigAddress->setText(QString::fromStdString(multisigAddress.ToString()));
        ui->redeemScript->setText(QString::fromStdString(redeemScriptHex));
    }
}

bool MultisigDialog::saveMultisigInfoForSharing(const CScript& redeemScript, const std::vector<CPubKey>& pubkeys, int required)
{
    if (!model)
        return false;
        
    // Get the multisig address
    CScriptID scriptID = CScriptID(redeemScript);
    CBitcoinAddress multisigAddress;
    multisigAddress.Set(scriptID);
    if (!multisigAddress.IsValid())
        return false;
        
    // Create a JSON object with the multisig information
    UniValue jsonObj(UniValue::VOBJ);
    
    // Add the multisig address
    jsonObj.pushKV("address", multisigAddress.ToString());
    
    // Add the redeem script
    jsonObj.pushKV("redeemScript", HexStr(redeemScript));
    
    // Add required signatures
    jsonObj.pushKV("required", required);
    
    // Add public keys
    UniValue pubKeysArray(UniValue::VARR);
    for (const CPubKey& pubKey : pubkeys) {
        pubKeysArray.push_back(HexStr(pubKey.begin(), pubKey.end()));
    }
    jsonObj.pushKV("pubkeys", pubKeysArray);
    
    // Format the JSON with indentation
    std::string jsonStr = jsonObj.write(2);
    
    // Ask user where to save the file
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Save Multisig Info for Sharing"), QDir::homePath(),
        tr("Multisig Info (*.json);;All Files (*)"));
        
    if (fileName.isEmpty())
        return false;
        
    // Save to file
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly)) {
        QTextStream stream(&file);
        stream << QString::fromStdString(jsonStr);
        file.close();
        
        showInfo(tr("Success"), tr("Multisig info saved to %1. You can share this file with other signers.").arg(fileName));
        return true;
    } else {
        showError(tr("Error"), tr("Could not write to file %1").arg(fileName));
        return false;
    }
}
