// Copyright (c) 2015-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "platformstyle.h"

#include "guiconstants.h"

#include <QApplication>
#include <QColor>
#include <QIcon>
#include <QImage>
#include <QPalette>
#include <QPixmap>
#include <QStyleFactory>


static const struct {
    const char *platformId;
    /** Show images on push buttons */
    const bool imagesOnButtons;
    /** Colorize single-color icons */
    const bool colorizeIcons;
    /** Extra padding/spacing in transactionview */
    const bool useExtraSpacing;
} platform_styles[] = {
    {"macosx", false, false, true},
    {"windows", true, false, false},
    /* Other: linux, unix, ... */
    {"other", true, true, false}
};
static const unsigned platform_styles_count = sizeof(platform_styles)/sizeof(*platform_styles);

namespace {
/* Local functions for colorizing single-color images */

void MakeSingleColorImage(QImage& img, const QColor& colorbase)
{
    img = img.convertToFormat(QImage::Format_ARGB32);
    for (int x = img.width(); x--; )
    {
        for (int y = img.height(); y--; )
        {
            const QRgb rgb = img.pixel(x, y);
            img.setPixel(x, y, qRgba(colorbase.red(), colorbase.green(), colorbase.blue(), qAlpha(rgb)));
        }
    }
}

QIcon ColorizeIcon(const QIcon& ico, const QColor& colorbase)
{
    QIcon new_ico;
    QSize sz;
    Q_FOREACH(sz, ico.availableSizes())
    {
        QImage img(ico.pixmap(sz).toImage());
        MakeSingleColorImage(img, colorbase);
        new_ico.addPixmap(QPixmap::fromImage(img));
    }
    return new_ico;
}

QImage ColorizeImage(const QString& filename, const QColor& colorbase)
{
    QImage img(filename);
    MakeSingleColorImage(img, colorbase);
    return img;
}

QIcon ColorizeIcon(const QString& filename, const QColor& colorbase)
{
    return QIcon(QPixmap::fromImage(ColorizeImage(filename, colorbase)));
}

}


PlatformStyle::PlatformStyle(const QString &_name, bool _imagesOnButtons, bool _colorizeIcons, bool _useExtraSpacing):
    name(_name),
    imagesOnButtons(_imagesOnButtons),
    colorizeIcons(true), // Always colorize icons for our theme
    useExtraSpacing(_useExtraSpacing),
    singleColor(THEME_GOLD),
    textColor(THEME_TEXT)
{
    // Set up dark theme palette
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, THEME_NAVY_BLUE);
    darkPalette.setColor(QPalette::WindowText, THEME_TEXT);
    darkPalette.setColor(QPalette::Base, THEME_NAVY_BLUE.darker(120));
    darkPalette.setColor(QPalette::AlternateBase, THEME_NAVY_BLUE);
    darkPalette.setColor(QPalette::ToolTipBase, THEME_GOLD);
    darkPalette.setColor(QPalette::ToolTipText, THEME_NAVY_BLUE);
    darkPalette.setColor(QPalette::Text, THEME_TEXT);
    darkPalette.setColor(QPalette::Button, THEME_NAVY_BLUE);
    darkPalette.setColor(QPalette::ButtonText, THEME_TEXT);
    darkPalette.setColor(QPalette::BrightText, THEME_GOLD);
    darkPalette.setColor(QPalette::Link, THEME_GOLD);
    darkPalette.setColor(QPalette::Highlight, THEME_GOLD);
    darkPalette.setColor(QPalette::HighlightedText, THEME_NAVY_BLUE);
    // Improve visibility for checkboxes and borders
    darkPalette.setColor(QPalette::Mid, THEME_BORDER);
    darkPalette.setColor(QPalette::Light, THEME_CHECKBOX);
    darkPalette.setColor(QPalette::Dark, THEME_BORDER);
    
    QApplication::setPalette(darkPalette);
    QApplication::setStyle(QStyleFactory::create("Fusion")); // Use Fusion style for better dark theme support

    // Checkbox and radio button styling is handled in bitcoin.cpp
}

QImage PlatformStyle::SingleColorImage(const QString& filename) const
{
    if (!colorizeIcons)
        return QImage(filename);
    return ColorizeImage(filename, SingleColor());
}

QIcon PlatformStyle::SingleColorIcon(const QString& filename) const
{
    if (!colorizeIcons)
        return QIcon(filename);
    return ColorizeIcon(filename, SingleColor());
}

QIcon PlatformStyle::SingleColorIcon(const QIcon& icon) const
{
    if (!colorizeIcons)
        return icon;
    return ColorizeIcon(icon, SingleColor());
}

QIcon PlatformStyle::TextColorIcon(const QString& filename) const
{
    return ColorizeIcon(filename, TextColor());
}

QIcon PlatformStyle::TextColorIcon(const QIcon& icon) const
{
    return ColorizeIcon(icon, TextColor());
}

const PlatformStyle *PlatformStyle::instantiate(const QString &platformId)
{
    for (unsigned x=0; x<platform_styles_count; ++x)
    {
        if (platformId == platform_styles[x].platformId)
        {
            return new PlatformStyle(
                    platform_styles[x].platformId,
                    platform_styles[x].imagesOnButtons,
                    platform_styles[x].colorizeIcons,
                    platform_styles[x].useExtraSpacing);
        }
    }
    return 0;
}
