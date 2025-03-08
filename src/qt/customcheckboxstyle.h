// Copyright (c) 2022 The Junkcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_CUSTOMCHECKBOXSTYLE_H
#define BITCOIN_QT_CUSTOMCHECKBOXSTYLE_H

#include <QProxyStyle>
#include <QPainter>
#include <QStyleOption>

/**
 * Custom style class that overrides the checkbox drawing to use a black checkmark
 */
class CustomCheckboxStyle : public QProxyStyle
{
public:
    CustomCheckboxStyle(QStyle* style = nullptr) : QProxyStyle(style) {}

    void drawPrimitive(PrimitiveElement element, const QStyleOption *option,
                      QPainter *painter, const QWidget *widget = nullptr) const override
    {
        // Override checkbox indicator drawing to use a black checkmark
        if (element == QStyle::PE_IndicatorCheckBox) {
            // Draw the checkbox background (white)
            QStyleOptionButton buttonOption;
            buttonOption.rect = option->rect;
            buttonOption.state = option->state;
            QProxyStyle::drawPrimitive(element, option, painter, widget);
            
            // If the checkbox is checked, draw a black X
            if (option->state & QStyle::State_On) {
                painter->save();
                painter->setPen(QPen(Qt::black, 2));
                QRect r = option->rect;
                painter->drawLine(r.x() + 3, r.y() + 3, r.x() + r.width() - 3, r.y() + r.height() - 3);
                painter->drawLine(r.x() + 3, r.y() + r.height() - 3, r.x() + r.width() - 3, r.y() + 3);
                painter->restore();
            }
            return;
        }
        QProxyStyle::drawPrimitive(element, option, painter, widget);
    }
};

#endif // BITCOIN_QT_CUSTOMCHECKBOXSTYLE_H
