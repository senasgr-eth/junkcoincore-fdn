
Debian
====================
This directory contains files used to package junkcoind/junkcoin-qt
for Debian-based Linux systems. If you compile junkcoind/junkcoin-qt yourself, there are some useful files here.

## junkcoin: URI support ##


junkcoin-qt.desktop  (Gnome / Open Desktop)
To install:

	sudo desktop-file-install junkcoin-qt.desktop
	sudo update-desktop-database

If you build yourself, you will either need to modify the paths in
the .desktop file or copy or symlink your junkcoin-qt binary to `/usr/bin`
and the `../../share/pixmaps/junkcoin128.png` to `/usr/share/pixmaps`

junkcoin-qt.protocol (KDE)

