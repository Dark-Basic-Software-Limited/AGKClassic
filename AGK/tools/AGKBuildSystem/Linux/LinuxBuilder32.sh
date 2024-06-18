#! /bin/sh

if [ `getconf LONG_BIT` != "32" ]
then
	echo "64bit build is now done by AGKBuild"
	exit
fi

set -e
cd ../../..
make clean
make -j2
cd apps/interpreter_linux
make clean
make
cd ../template_linux
make clean
make
cd ../../CompilerNew
make clean
make
cd ../Broadcaster/AGKBroadcaster
make clean
make
cd ../../IDE/Geany-1.24.1
make
make install
cd ../..


mkdir -p /home/test/NetworkShare/Release32
cp platform/linux/Lib/Release32/libAGKLinux.a /home/test/NetworkShare/Release32/libAGKLinux.a
mkdir -p /home/test/NetworkShare/Compiler
cp CompilerNew/build/AGKCompiler32 /home/test/NetworkShare/Compiler/AGKCompiler32
cp Broadcaster/AGKBroadcaster/build/AGKBroadcaster32 /home/test/NetworkShare/Compiler/AGKBroadcaster32
mkdir -p /home/test/NetworkShare/Compiler/interpreters
cp apps/interpreter_linux/build/LinuxPlayer32 /home/test/NetworkShare/Compiler/interpreters/LinuxPlayer32
mkdir -p /home/test/NetworkShare/Editor/bin
cp /home/test/Programs/AGKIDE/bin/geany /home/test/NetworkShare/Editor/bin/geany32
mkdir -p /home/test/NetworkShare/Editor/lib/geany
cp /home/test/Programs/AGKIDE/lib/geany/splitwindow.so /home/test/NetworkShare/Editor/lib/geany/splitwindow32.so
cp /home/test/Programs/AGKIDE/lib/geany/saveactions.so /home/test/NetworkShare/Editor/lib/geany/saveactions32.so
