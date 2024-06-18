#! /bin/sh

cd /Users/paultgc/Projects/GeanyBundle
~/.new_local/bin/jhbuild run ~/.local/bin/gtk-mac-bundler AppGameKit.bundle

# copy files that for some reason the build misses
cp /Users/paultgc/Documents/SVN/AGK2/IDE/Geany-1.24.1/build-extra/gdk-pixbuf.loaders /Users/paultgc/Projects/AGKMac/AppGameKit.app/Contents/Resources/etc/gtk-2.0/gdk-pixbuf.loaders

cp /Users/paultgc/Documents/SVN/AGK2/IDE/Geany-1.24.1/build-extra/icon-theme.cache /Users/paultgc/Projects/AGKMac/AppGameKit.app/Contents/Resources/share/icons/hicolor/icon-theme.cache

cp /Users/paultgc/Documents/SVN/AGK2/IDE/Geany-1.24.1/build-extra/index.theme /Users/paultgc/Projects/AGKMac/AppGameKit.app/Contents/Resources/share/icons/hicolor/index.theme
