#! /bin/sh

set -e
cd ../../..
make -f PiMakefile clean
make -j4 -f PiMakefile
cd apps/interpreter_pi
make clean
make
cd ../../CompilerNew
make -f PiMakefile clean
make -f PiMakefile
cd ../Broadcaster/AGKBroadcaster
make -f PiMakefile clean
make -f PiMakefile
cd ../../IDE/Geany-1.24.1
make
make install
cd ../..

# make directories
mkdir -p /home/pi/Programs/AGKPi
rm -rf /home/pi/Programs/AGKPi/*
mkdir -p /home/pi/Programs/AGKPi/Tier1/Compiler/interpreters
mkdir -p /home/pi/Programs/AGKPi/Tier1/Editor
mkdir -p /home/pi/Programs/AGKPi/Players/RaspPi
mkdir -p /home/pi/Programs/AGKPi/Projects
mkdir -p /home/pi/Programs/AGKPi/Tier2/platform/pi/Lib/Release
mkdir -p /home/pi/Programs/AGKPi/Tier2/platform/pi/Source
mkdir -p /home/pi/Programs/AGKPi/Tier2/common/include
mkdir -p /home/pi/Programs/AGKPi/Tier2/common/Collision
mkdir -p /home/pi/Programs/AGKPi/Tier2/bullet
mkdir -p /home/pi/Programs/AGKPi/Tier2/apps/interpreter
mkdir -p /home/pi/Programs/AGKPi/Tier2/apps/interpreter_pi
mkdir -p /home/pi/Programs/AGKPi/Tier2/apps/template_pi
mkdir -p /home/pi/Programs/AGKPi/Tier2/apps/template_pi_console

# copy compiler and broadcaster
cp CompilerNew/build/AGKCompiler /home/pi/Programs/AGKPi/Tier1/Compiler/AGKCompiler
#cp CompilerNew/CommandList.dat /home/pi/Programs/AGKPi/Tier1/Compiler/CommandList.dat
cp Broadcaster/AGKBroadcaster/build/AGKBroadcaster /home/pi/Programs/AGKPi/Tier1/Compiler/AGKBroadcaster

# copy interpreter
cp apps/interpreter_pi/build/PiPlayer /home/pi/Programs/AGKPi/Tier1/Compiler/interpreters/PiPlayer
cp apps/interpreter_pi/build/PiPlayer /home/pi/Programs/AGKPi/Players/RaspPi/PiPlayer

# copy IDE
cp -r /home/pi/Programs/AGKIDE/* /home/pi/Programs/AGKPi/Tier1/Editor/

# copy tier 2 files
cp platform/pi/Lib/Release/libAGKPi.a /home/pi/Programs/AGKPi/Tier2/platform/pi/Lib/Release/libAGKPi.a
cp platform/pi/Source/PiNetwork.h /home/pi/Programs/AGKPi/Tier2/platform/pi/Source/PiNetwork.h

# copy header files
cd common
find ./include -name '*.h' -print0 | rsync --files-from=- --from0 . /home/pi/Programs/AGKPi/Tier2/common/
find ./Collision -name '*.h' -print0 | rsync --files-from=- --from0 . /home/pi/Programs/AGKPi/Tier2/common/
cd ../bullet
find . -name '*.h' -print0 | rsync --files-from=- --from0 . /home/pi/Programs/AGKPi/Tier2/bullet/

# copy apps
cd ../apps
cp interpreter_pi/Core.cpp /home/pi/Programs/AGKPi/Tier2/apps/interpreter_pi/Core.cpp
cp interpreter_pi/Makefile /home/pi/Programs/AGKPi/Tier2/apps/interpreter_pi/Makefile
cp template_pi/Core.cpp /home/pi/Programs/AGKPi/Tier2/apps/template_pi/Core.cpp
cp template_pi/Makefile /home/pi/Programs/AGKPi/Tier2/apps/template_pi/Makefile
cp template_pi/template.cpp /home/pi/Programs/AGKPi/Tier2/apps/template_pi/template.cpp
cp template_pi/template.h /home/pi/Programs/AGKPi/Tier2/apps/template_pi/template.h
cp template_pi_console/Core.cpp /home/pi/Programs/AGKPi/Tier2/apps/template_pi_console/Core.cpp
cp template_pi_console/Makefile /home/pi/Programs/AGKPi/Tier2/apps/template_pi_console/Makefile
cp template_pi_console/template.cpp /home/pi/Programs/AGKPi/Tier2/apps/template_pi_console/template.cpp
cp template_pi_console/template.h /home/pi/Programs/AGKPi/Tier2/apps/template_pi_console/template.h
cd interpreter
find . -name '*.h' -print0 | rsync --files-from=- --from0 . /home/pi/Programs/AGKPi/Tier2/apps/interpreter/
find . -name '*.cpp' -print0 | rsync --files-from=- --from0 . /home/pi/Programs/AGKPi/Tier2/apps/interpreter/
cd ../..

# copy Projects
cp -r Examples/* /home/pi/Programs/AGKPi/Projects/

# create shortcut
cd /home/pi/Programs/AGKPi
ln -s Tier1/Editor/bin/geany AGK

