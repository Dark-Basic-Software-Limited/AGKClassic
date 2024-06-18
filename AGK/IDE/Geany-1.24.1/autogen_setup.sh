#!/bin/sh

export PKG_CONFIG_PATH="../../../../../gtk/source/gtk+-2.24.32:../../../../../gtk/source/pango-1.46.2:../../../../../gtk/source/glib-2.66.0:../../../../../gtk/source/cairo-1.16.0:../../../../../gtk/source/pixman-0.40.0:../../../../../gtk/source/libpng-1.6.37:../../../../../gtk/source/gdk-pixbuf-2.40.0:../../../../../gtk/source/atk-2.36.0"

export CXXFLAGS="-O2 -I$HOME/gtk/inst/include -I$HOME/gtk/inst/include/harfbuzz -I$HOME/gtk/inst/include/atk-1.0 -x objective-c++ -mmacosx-version-min=10.9 -march=x86-64"

export CFLAGS="-O2 -I$HOME/gtk/inst/include -I$HOME/gtk/inst/include/harfbuzz -I$HOME/gtk/inst/include/atk-1.0 -x objective-c -mmacosx-version-min=10.9 -march=x86-64"

export LDFLAGS="-O2 -mmacosx-version-min=10.9 -L$HOME/gtk/inst/lib"

export LIBS="-lgtkmacintegration-gtk2 -framework ApplicationServices -framework CoreFoundation -framework Cocoa -lgtk-quartz-2.0 -latk-1.0 -lpixman-1 -lz -lgdk_pixbuf-2.0 -lm -lpng16"

./autogen.sh --prefix="$HOME/Projects/Geany-Compiled" --disable-vte --disable-dependency-tracking
