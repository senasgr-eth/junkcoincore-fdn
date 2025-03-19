#ifndef BITCOIN_QT_MULTISIGDIALOG_H
#define BITCOIN_QT_MULTISIGDIALOG_H

#include <QDialog>
#include <QMessageBox>
#include <QString>
#include <QStringList>
#include <QComboBox>
#include <QTableWidgetItem>
#include <QMap>
#include "primitives/transaction.h"
#include "script/script.h"
#include "base58.h"

class PlatformStyle;
class WalletModel;

namespace Ui {
    class MultisigDialog;
}

class MultisigDialog : public QDialog
{
    Q_OBJECT

public:
    enum Mode {
        CreateWallet,
        ImportWallet,
        SignTransaction,
        BroadcastTransaction,
        ListAddresses
    };

    enum InputType {
        INPUT_PUBKEY,
        INPUT_ADDRESS
    };

    explicit MultisigDialog(const PlatformStyle *pplatformStyle, QWidget *parent = nullptr);
    ~MultisigDialog();

    void setModel(WalletModel *pwalletModel);
    void addWalletModel(const QString& name, WalletModel *walletModel);
    void showTab(Mode mode);

private Q_SLOTS:
    void createMultisigWallet();
    void importMultisigWallet();
    void signMultisigTx();
    void broadcastMultisigTx();
    void clearAll();
    void updateClearButton();
    void addInputRow();
    void removeInputRow();
    void validateInput(int nRow);
    void onInputTypeChanged(int nRow);
    void onTableCellChanged(int nRow, int nColumn);
    void loadTransaction();
    void saveTransaction();
    void loadSignedTransaction();
    void onWalletSelectionChanged(int index);
    void refreshAddressList();
    void populateAddressSelector();
    void addSelectedAddress();
    void onTotalSignaturesChanged(int value);
    void showAddressMenu();
    void loadMultisigInfoFromFile();
    void decodeTransaction();

private:
    Ui::MultisigDialog *ui;
    WalletModel *model;
    const PlatformStyle *platformStyle;
    Mode currentMode;
    QMap<QString, WalletModel*> mapWalletModels;
    CMutableTransaction currentTransaction; // Store the current transaction
    std::set<uint256> broadcastTransactions; // Set of transaction hashes that have been broadcast

    bool createRedeemScript(CScript& scriptRedeem, QString& strErrorMsg);
    bool importRedeemScript(const QString& strScriptHex, QString& strErrorMsg);
    bool createRawTransaction(CMutableTransaction& txNew, QString& strErrorMsg);
    bool signRawTransaction(CMutableTransaction& tx, QString& strErrorMsg);
    bool broadcastTransaction(const CMutableTransaction& tx, QString& strErrorMsg);
    bool validateAddress(const QString& strAddress, QString& strPubKey, QString& strError);
    bool saveMultisigInfoForSharing(const CScript& redeemScript, const std::vector<CPubKey>& pubkeys, int required);
    bool getPubKeyFromAddress(const QString& strAddress, QString& strPubKey);
    void setupInputRow(int nRow);
    void updateInputStatus(int nRow, const QString& strStatus, bool fValid);
    bool hasSignerPubKey(const QString& strPubKey);
    bool hasPrivateKey(const std::vector<CPubKey>& pubkeys);
    bool decodeAndDisplayTransaction(const QString& strHexTx);
    void importMultisigScript(const CScript& scriptRedeem, bool fImportAsReadOnly);
    std::vector<std::string> getValidatedInputs(QString& strErrorMsg);
    void loadMultisigAddressTable();
    void populateWalletAddressSelector();
    void populateSigningAddressSelector();

    void showError(const QString& strTitle, const QString& strMessage);
    void showInfo(const QString& strTitle, const QString& strMessage);
};

#endif // BITCOIN_QT_MULTISIGDIALOG_H
