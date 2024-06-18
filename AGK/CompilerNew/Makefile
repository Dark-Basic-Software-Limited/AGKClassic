ARCH := $(shell getconf LONG_BIT)

CC = g++
ODIR = build/obj$(ARCH)
INC = 
LDFLAGS = 
CFLAGS = -O2

_OBJS = AGKCompiler2.o \
        cFile.o \
        DynArray.o \
        Parser1.o \
        Parser2.o \
        ProgramData.o \
        uString.o \
        LinuxCompiler.o 
OBJS = $(patsubst %,$(ODIR)/%,$(_OBJS))

all: setup $(OBJS) Executable

trial: trialflags all
	
trialflags:
	$(eval CFLAGS += -DFREEVERSION)
	
weekend: weekendflags all

weekendflags:
	$(eval CFLAGS += -DAGK_STEAM_WEEKEND)
	$(eval INC += -I /home/paul/Downloads/sdk/public)
	$(eval LDFLAGS += -L/home/paul/Downloads/sdk/redistributable_bin/linux64 -lsteam_api -Wl,-rpath='$$$$ORIGIN')

static: staticflags setup $(OBJS) linkall

static flags:
	$(eval CFLAGS += -DAGK_STATIC_LIB)

setup:
	mkdir -p build/obj$(ARCH)

$(ODIR)/%.o: %.cpp
	$(CC) -DIDE_LINUX -DAGK_COMPILER -no-pie -c $(INC) $(CFLAGS) -o $@ $<
	
$(ODIR)/%.o : ../common/Source/%.cpp
	$(CC) -DIDE_LINUX -DAGK_COMPILER -no-pie -c $(INC) $(CFLAGS) -o $@ $<
	
$(ODIR)/%.o: ../interpreter/%.cpp
	$(CC) -DIDE_LINUX -DAGK_COMPILER -no-pie -c $(INC) $(CFLAGS) -o $@ $<
	
Executable: | $(OBJS)
	$(CC) $(OBJS) -o build/AGKCompiler$(ARCH) $(LDFLAGS)
	
linkall: | $(OBJS)
	ar rcs build/libLinuxCompiler$(ARCH).a $(OBJS)

clean:
	rm -rf build/obj$(ARCH)/*
	rm -f build/AGKCompiler$(ARCH)
	rm -f build/libLinuxCompiler$(ARCH).a
