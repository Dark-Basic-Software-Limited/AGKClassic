#! /bin/sh

cd /Users/michaeljohnson/Projects/GeanyBundle
export PATH=$PREFIX/bin:~/.local/bin:$PATH
~/.local/bin/jhbuild run gtk-mac-bundler AppGameKitTrial.bundle
