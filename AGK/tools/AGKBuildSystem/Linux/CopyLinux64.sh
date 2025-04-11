#! /bin/sh

set -e
mkdir -p /home/paul/Programs/AGKLinux
rm -rf /home/paul/Programs/AGKLinux/*

# copy files from Windows build
echo "  Copying from Windows shared folder"
cp -r /home/paul/Shares/FilesAlphaLinux/AGK/* /home/paul/Programs/AGKLinux/

# copy projects
mkdir -p /home/paul/Programs/AGKLinux/Projects
rm -rf /home/paul/Programs/AGKLinux/Projects/*
cp -r Examples/* /home/paul/Programs/AGKLinux/Projects/

# copy Android files to IDE folder
echo "  Copying Android files to IDE"
rm -rf /home/paul/Programs/AGK/Tier1/Editor/share/geany/android/sourceGoogle/*
rm -rf /home/paul/Programs/AGK/Tier1/Editor/share/geany/android/sourceAmazon/*
rm -rf /home/paul/Programs/AGK/Tier1/Editor/share/geany/android/sourceOuya/*
rm -rf /home/paul/Programs/AGK/Tier1/Editor/share/geany/android/lib/*
cp -r /home/paul/Shares/WindowsReceive/Classic/Android/* /home/paul/Programs/AGK/Tier1/Editor/share/geany/android/

# copy compilers and command list
echo "  Copying compiler and broadcaster"
mkdir -p /home/paul/Programs/AGKLinux/Tier1/Compiler/interpreters
mkdir -p /home/paul/Programs/AGKLinux/Tier1/Compiler/Plugins
cp CompilerNew/build/AGKCompiler64 /home/paul/Programs/AGKLinux/Tier1/Compiler/AGKCompiler64
cp /home/paul/Receive/Compiler/AGKCompiler32 /home/paul/Programs/AGKLinux/Tier1/Compiler/AGKCompiler32
#cp CompilerNew/CommandList.dat /home/paul/Programs/AGKLinux/Tier1/Compiler/CommandList.dat
cp -r plugins/Plugins/* /home/paul/Programs/AGKLinux/Tier1/Compiler/Plugins

# copy broadcasters
cp Broadcaster/AGKBroadcaster/build/AGKBroadcaster64 /home/paul/Programs/AGKLinux/Tier1/Compiler/AGKBroadcaster64
cp /home/paul/Receive/Compiler/AGKBroadcaster32 /home/paul/Programs/AGKLinux/Tier1/Compiler/AGKBroadcaster32

# copy interpreters
cp apps/interpreter_linux/build/LinuxPlayer64 /home/paul/Programs/AGKLinux/Tier1/Compiler/interpreters/LinuxPlayer64
cp /home/paul/Receive/Compiler/interpreters/LinuxPlayer32 /home/paul/Programs/AGKLinux/Tier1/Compiler/interpreters/LinuxPlayer32

# copy AGK libs
echo "  Copying Tier 2 libs"
cp /home/paul/Receive/Release32/libAGKLinux.a /home/paul/Programs/AGKLinux/Tier2/platform/linux/Lib/Release32/libAGKLinux.a
cp platform/linux/Lib/Release64/libAGKLinux.a /home/paul/Programs/AGKLinux/Tier2/platform/linux/Lib/Release64/libAGKLinux.a
cp platform/html5/Lib/Release/libAGKHTML5.a /home/paul/Programs/AGKLinux/Tier2/platform/html5/Lib/Release/libAGKHTML5.a

# copy Linux players to Players folder
echo "  Copying players"
cp apps/interpreter_linux/build/LinuxPlayer64 /home/paul/Programs/AGKLinux/Players/Linux/LinuxPlayer64
cp /home/paul/Receive/Compiler/interpreters/LinuxPlayer32 /home/paul/Programs/AGKLinux/Players/Linux/LinuxPlayer32

# copy IDE to build folder, separate executable into 32 and 64 bit versions
echo "  Copying IDE"
cp -r /home/paul/Programs/AGK/Tier1/* /home/paul/Programs/AGKLinux/Tier1/
mv /home/paul/Programs/AGKLinux/Tier1/Editor/bin/geany /home/paul/Programs/AGKLinux/Tier1/Editor/bin/geany64
cp /home/paul/Receive/Editor/bin/geany32 /home/paul/Programs/AGKLinux/Tier1/Editor/bin/geany32

# copy plugins for 32 bit version
cp /home/paul/Receive/Editor/lib/geany/saveactions32.so /home/paul/Programs/AGKLinux/Tier1/Editor/lib/geany/saveactions32.so
cp /home/paul/Receive/Editor/lib/geany/splitwindow32.so /home/paul/Programs/AGKLinux/Tier1/Editor/lib/geany/splitwindow32.so

# copy interpreters to Windows
echo "  Copying Linux files to Windows shared folder"
cp apps/interpreter_linux/build/LinuxPlayer64 /home/paul/Shares/WindowsReceive/Classic/LinuxPlayer64
cp /home/paul/Receive/Compiler/interpreters/LinuxPlayer32 /home/paul/Shares/WindowsReceive/Classic/LinuxPlayer32

# copy HTML5 files to Windows
echo "  Copying HTML5 files to Windows shared folder"
cp -r IDE/Geany-1.24.1/data/html5/* /home/paul/Shares/WindowsReceive/Classic/HTML5/

# create shortcuts for IDE executable
echo "  Creating shortcuts"
cd /home/paul/Programs/AGKLinux
ln -s Tier1/Editor/bin/geany32 AGK32
ln -s Tier1/Editor/bin/geany64 AGK64
