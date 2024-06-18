#! /bin/sh

set -e
mkdir -p /home/michael/AGKClassicBuild
rm -rf /home/michael/AGKClassicBuild/*

# copy files from Windows build
echo "  Copying from Windows shared folder"
cp -r /home/michael/AGK/Shared/AlphaFilesLinux/AGK/* /home/michael/AGKClassicBuild/

# copy projects
mkdir -p /home/michael/AGKClassicBuild/Projects
rm -rf /home/michael/AGKClassicBuild/Projects/*
cp -r Examples/* /home/michael/AGKClassicBuild/Projects/

# copy Android files to IDE folder
echo "  Copying Android files to IDE"
rm -rf /home/michael/AGKClassicBuild/Tier1/Editor/share/geany/android/sourceGoogle/resMerged/*
rm -rf /home/michael/AGKClassicBuild/Tier1/Editor/share/geany/android/sourceAmazon/resMerged/*
rm -rf /home/michael/AGKClassicBuild/Tier1/Editor/share/geany/android/sourceOuya/resMerged/*
rm -rf /home/michael/AGKClassicBuild/Tier1/Editor/share/geany/android/sourceGoogle/resOrig/*
rm -rf /home/michael/AGKClassicBuild/Tier1/Editor/share/geany/android/sourceAmazon/resOrig/*
rm -rf /home/michael/AGKClassicBuild/Tier1/Editor/share/geany/android/sourceOuya/resOrig/*

mkdir -p /home/michael/AGKClassicBuild/Tier1/Editor/share/geany
mkdir -p /home/michael/AGKClassicBuild/Tier1/Editor/share/geany/android
cp -r /home/michael/AGK/Shared/WindowsReceive/Classic/Android/* /home/michael/AGKClassicBuild/Tier1/Editor/share/geany/android/



# copy compilers and command list
echo "  Copying compiler and broadcaster"
mkdir -p /home/michael/AGKClassicBuild/Tier1/Compiler/interpreters
mkdir -p /home/michael/AGKClassicBuild/Tier1/Compiler/Plugins
cp CompilerNew/build/AGKCompiler64 /home/michael/AGKClassicBuild/Tier1/Compiler/AGKCompiler64
#cp /home/paul/Receive/Compiler/AGKCompiler32 /home/AGKClassicBuild/AGKLinux/Tier1/Compiler/AGKCompiler32
#cp CompilerNew/CommandList.dat /home/paul/Programs/AGKLinux/Tier1/Compiler/CommandList.dat
cp -r plugins/Plugins/* /home/michael/AGKClassicBuild/Tier1/Compiler/Plugins

# copy broadcasters
cp Broadcaster/AGKBroadcaster/build/AGKBroadcaster64 /home/michael/AGKClassicBuild/Tier1/Compiler/AGKBroadcaster64
#cp /home/michael/Receive/Compiler/AGKBroadcaster32 /home/AGKClassicBuild/Tier1/Compiler/AGKBroadcaster32

# copy interpreters
cp apps/interpreter_linux/build/LinuxPlayer64 /home/michael/AGKClassicBuild/Tier1/Compiler/interpreters/LinuxPlayer64
#cp /home/michael/Receive/Compiler/interpreters/LinuxPlayer32 /home/AGKClassicBuild/Tier1/Compiler/interpreters/LinuxPlayer32

# copy AGK libs
echo "  Copying Tier 2 libs"
#cp /home/michael/Receive/Release32/libAGKLinux.a /home/michael/AGKClassicBuild/Tier2/platform/linux/Lib/Release32/libAGKLinux.a

mkdir -p /home/michael/AGKClassicBuild/Tier2/platform/linux
mkdir -p /home/michael/AGKClassicBuild/Tier2/platform/linux/Lib
mkdir -p /home/michael/AGKClassicBuild/Tier2/platform/linux/Lib/Release64

mkdir -p /home/michael/AGKClassicBuild/Tier2/platform/html5
mkdir -p /home/michael/AGKClassicBuild/Tier2/platform/html5/Lib
mkdir -p /home/michael/AGKClassicBuild/Tier2/platform/html5/Lib/Release

cp platform/linux/Lib/Release64/libAGKLinux.a /home/michael/AGKClassicBuild/Tier2/platform/linux/Lib/Release64/libAGKLinux.a
cp platform/html5/Lib/Release/libAGKHTML5.a /home/michael/AGKClassicBuild/Tier2/platform/html5/Lib/Release/libAGKHTML5.a

# copy Linux players to Players folder
echo "  Copying players"
cp apps/interpreter_linux/build/LinuxPlayer64 /home/michael/AGKClassicBuild/Players/Linux/LinuxPlayer64
#cp /home/michael/Receive/Compiler/interpreters/LinuxPlayer32 /home/michael/AGKClassicBuild/Players/Linux/LinuxPlayer32

# copy IDE to build folder, separate executable into 32 and 64 bit versions
echo "  Copying IDE"
cp -r /home/michael/Programs/AGKIDE/* /home/michael/AGKClassicBuild/Tier1/Editor
#mv /home/michael/Programs/AGKIDE/bin/geany /home/michael/AGKClassicBuild/AGKLinux/Tier1/Editor/bin/geany64
#cp /home/paul/Receive/Editor/bin/geany32 /home/michael/AGKClassicBuild/Tier1/Editor/bin/geany32

# copy plugins for 32 bit version
#cp /home/michael/Receive/Editor/lib/geany/saveactions32.so /home/michael/Programs/AGKLinux/Tier1/Editor/lib/geany/saveactions32.so
#cp /home/michael/Receive/Editor/lib/geany/splitwindow32.so /home/michael/Programs/AGKLinux/Tier1/Editor/lib/geany/splitwindow32.so

# copy interpreters to Windows
echo "  Copying Linux files to Windows shared folder"
cp apps/interpreter_linux/build/LinuxPlayer64 /home/michael/AGK/Shared/WindowsReceive/Classic/LinuxPlayer64
#cp /home/michael/Receive/Compiler/interpreters/LinuxPlayer32 /home/michael/AGK/Shared/WindowsReceive/Classic/LinuxPlayer32

# copy HTML5 files to Windows
echo "  Copying HTML5 files to Windows shared folder"
cp -r IDE/Geany-1.24.1/data/html5/* /home/michael/AGK/Shared/WindowsReceive/Classic/HTML5/

# create shortcuts for IDE executable
echo "  Creating shortcuts"
cd /home/michael/AGKClassicBuild
#ln -s Tier1/Editor/bin/geany32 AGK32
#ln -s Tier1/Editor/bin/geany64 AGK64
ln -s Tier1/Editor/bin/geany AGK
