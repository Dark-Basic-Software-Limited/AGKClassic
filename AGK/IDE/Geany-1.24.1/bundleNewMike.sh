#! /bin/bash

cd /Users/michaeljohnson/Projects/GeanyBundle
bash ~/.new_local/bin/jhbuild run ~/.local/bin/gtk-mac-bundler AppGameKit.bundle

# copy files that for some reason the build misses
cp /Users/michaeljohnson/Documents/Projects/AGK/Classic/IDE/Geany-1.24.1/build-extra/gdk-pixbuf.loaders /Users/michaeljohnson/Projects/AGKMac/AppGameKit.app/Contents/Resources/etc/gtk-2.0/gdk-pixbuf.loaders

cp /Users/michaeljohnson/Documents/Projects/AGK/Classic/IDE/Geany-1.24.1/build-extra/icon-theme.cache /Users/michaeljohnson/Projects/AGKMac/AppGameKit.app/Contents/Resources/share/icons/hicolor/icon-theme.cache

cp /Users/michaeljohnson/Documents/Projects/AGK/Classic/IDE/Geany-1.24.1/build-extra/index.theme /Users/michaeljohnson/Projects/AGKMac/AppGameKit.app/Contents/Resources/share/icons/hicolor/index.theme
