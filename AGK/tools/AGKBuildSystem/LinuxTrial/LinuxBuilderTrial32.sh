#! /bin/sh

if [ `getconf LONG_BIT` != "32" ]
then
	echo "64bit build is now done by AGKBuild"
	exit
fi

set -e
cd ../../..
make clean
make -j2 trial
cd apps/interpreter_linux
make clean
make trial
cd ../../CompilerNew
make clean
make trial
cd ../Broadcaster/AGKBroadcaster
make clean
make
cd ../../IDE/Geany-1.24.1
make
make install
cd ../..


mkdir -p /home/test/NetworkShare/Release32Trial
cp platform/linux/Lib/Release32Trial/libAGKLinux.a /home/test/NetworkShare/Release32Trial/libAGKLinux.a

mkdir -p /home/test/NetworkShare/CompilerTrial
cp CompilerNew/build/AGKCompiler32 /home/test/NetworkShare/CompilerTrial/AGKCompiler32
cp Broadcaster/AGKBroadcaster/build/AGKBroadcaster32 /home/test/NetworkShare/CompilerTrial/AGKBroadcaster32

mkdir -p /home/test/NetworkShare/CompilerTrial/interpreters
cp apps/interpreter_linux/build/LinuxPlayer32 /home/test/NetworkShare/CompilerTrial/interpreters/LinuxPlayer32

mkdir -p /home/test/NetworkShare/EditorTrial/bin
cp /home/test/Programs/AGKIDE/bin/geany /home/test/NetworkShare/EditorTrial/bin/geany32

mkdir -p /home/test/NetworkShare/EditorTrial/lib/geany
cp /home/test/Programs/AGKIDE/lib/geany/splitwindow.so /home/test/NetworkShare/EditorTrial/lib/geany/splitwindow32.so
cp /home/test/Programs/AGKIDE/lib/geany/saveactions.so /home/test/NetworkShare/EditorTrial/lib/geany/saveactions32.so
