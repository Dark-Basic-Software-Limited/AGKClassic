#! /bin/sh

set -e
mkdir -p /home/paul/Programs/AGKLinuxTrial
rm -rf /home/paul/Programs/AGKLinuxTrial/*

# copy files from Windows build
echo "  Copying from Windows shared folder"
mkdir -p /home/paul/Programs/AGKLinuxTrial/Tier2/apps/template_linux
mkdir -p /home/paul/Programs/AGKLinuxTrial/Tier2/bullet
mkdir -p /home/paul/Programs/AGKLinuxTrial/Tier2/common
mkdir -p /home/paul/Programs/AGKLinuxTrial/Tier2/platform/linux
cp -r /home/paul/Shares/FilesAlphaLinux/AGK/Tier2/apps/template_linux/* /home/paul/Programs/AGKLinuxTrial/Tier2/apps/template_linux/
cp -r /home/paul/Shares/FilesAlphaLinux/AGK/Tier2/bullet/* /home/paul/Programs/AGKLinuxTrial/Tier2/bullet/
cp -r /home/paul/Shares/FilesAlphaLinux/AGK/Tier2/common/* /home/paul/Programs/AGKLinuxTrial/Tier2/common/
cp -r /home/paul/Shares/FilesAlphaLinux/AGK/Tier2/platform/linux/* /home/paul/Programs/AGKLinuxTrial/Tier2/platform/linux/

# copy readme and change log
cp /home/paul/Shares/FilesAlphaLinux/AGK/ReadMe /home/paul/Programs/AGKLinuxTrial/ReadMe
cp /home/paul/Shares/FilesAlphaLinux/AGK/ChangeLog.txt /home/paul/Programs/AGKLinuxTrial/ChangeLog.txt

# copy help files
echo "  Copying help files"
mkdir -p /home/paul/Programs/AGKLinuxTrial/Tier1/Help
cp -r /home/paul/Shares/FilesAlphaLinux/AGK/Tier1/Help/* /home/paul/Programs/AGKLinuxTrial/Tier1/Help/

# copy projects
echo "  Copying projects"
mkdir -p /home/paul/Programs/AGKLinuxTrial/Projects
cp -r Examples/* /home/paul/Programs/AGKLinuxTrial/Projects/

# remove export files from IDE folder
echo "  Removing Android files from IDE"
rm -rf /home/paul/Programs/AGK/Tier1/Editor/share/geany/android/*
echo "  Removing iOS files from IDE"
rm -rf /home/paul/Programs/AGK/Tier1/Editor/share/geany/ios/*
echo "  Removing HTML5 files from IDE"
rm -rf /home/paul/Programs/AGK/Tier1/Editor/share/geany/html5/*

# copy compilers and command list
echo "  Copying compiler and broadcaster"
mkdir -p /home/paul/Programs/AGKLinuxTrial/Tier1/Compiler/interpreters
cp CompilerNew/build/AGKCompiler64 /home/paul/Programs/AGKLinuxTrial/Tier1/Compiler/AGKCompiler64
cp /home/paul/Receive/CompilerTrial/AGKCompiler32 /home/paul/Programs/AGKLinuxTrial/Tier1/Compiler/AGKCompiler32
cp CompilerNew/CommandList.dat /home/paul/Programs/AGKLinuxTrial/Tier1/Compiler/CommandList.dat

# copy broadcasters
cp Broadcaster/AGKBroadcaster/build/AGKBroadcaster64 /home/paul/Programs/AGKLinuxTrial/Tier1/Compiler/AGKBroadcaster64
cp /home/paul/Receive/CompilerTrial/AGKBroadcaster32 /home/paul/Programs/AGKLinuxTrial/Tier1/Compiler/AGKBroadcaster32

# copy interpreters
cp apps/interpreter_linux/build/LinuxPlayer64 /home/paul/Programs/AGKLinuxTrial/Tier1/Compiler/interpreters/LinuxPlayer64
cp /home/paul/Receive/CompilerTrial/interpreters/LinuxPlayer32 /home/paul/Programs/AGKLinuxTrial/Tier1/Compiler/interpreters/LinuxPlayer32

# copy AGK libs
echo "  Copying Tier 2 libs"
cp /home/paul/Receive/Release32Trial/libAGKLinux.a /home/paul/Programs/AGKLinuxTrial/Tier2/platform/linux/Lib/Release32/libAGKLinux.a
cp platform/linux/Lib/Release64Trial/libAGKLinux.a /home/paul/Programs/AGKLinuxTrial/Tier2/platform/linux/Lib/Release64/libAGKLinux.a

# copy IDE to build folder, separate executable into 32 and 64 bit versions
echo "  Copying IDE"
cp -r /home/paul/Programs/AGK/Tier1/* /home/paul/Programs/AGKLinuxTrial/Tier1/
mv /home/paul/Programs/AGKLinuxTrial/Tier1/Editor/bin/geany /home/paul/Programs/AGKLinuxTrial/Tier1/Editor/bin/geany64
cp /home/paul/Receive/EditorTrial/bin/geany32 /home/paul/Programs/AGKLinuxTrial/Tier1/Editor/bin/geany32

# copy plugins for 32 bit version
cp /home/paul/Receive/EditorTrial/lib/geany/saveactions32.so /home/paul/Programs/AGKLinuxTrial/Tier1/Editor/lib/geany/saveactions32.so
cp /home/paul/Receive/EditorTrial/lib/geany/splitwindow32.so /home/paul/Programs/AGKLinuxTrial/Tier1/Editor/lib/geany/splitwindow32.so

# create shortcuts for IDE executable
echo "  Creating shortcuts"
cd /home/paul/Programs/AGKLinuxTrial
ln -s Tier1/Editor/bin/geany32 AGK32
ln -s Tier1/Editor/bin/geany64 AGK64
