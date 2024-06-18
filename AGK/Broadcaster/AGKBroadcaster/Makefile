ARCH := $(shell getconf LONG_BIT)

CC = g++
ODIR = build/obj$(ARCH)
INC = -I../../common/include -I../../common -I../interpreter -I../../bullet -I../../bullet/BulletCollision/CollisionShapes
CFLAGS = -O2

_OBJS = LinuxCore.o \
        Listener.o \
        Device.o \
        AGKBroadcaster.o 
OBJS = $(patsubst %,$(ODIR)/%,$(_OBJS))

all: setup $(OBJS) Executable

setup:
	mkdir -p build/obj$(ARCH)

$(ODIR)/%.o: %.cpp
	$(CC) -DAGK_LINUX -c $(INC) -o $@ $< $(CFLAGS)
	
Executable: | $(OBJS)
	$(CC) $(OBJS) -o build/AGKBroadcaster$(ARCH) -L../../platform/linux/Lib/Release$(ARCH) -Wl,-Bstatic -lAGKLinux -lglfw3 -Wl,-Bdynamic -lGL -lXt -lX11 -lXxf86vm -lXrandr -lpthread -lXi -lXinerama -lXcursor -lopenal -ludev -ldl

clean:
	rm -rf build/obj$(ARCH)/*
	rm -f build/AGKBroadcaster$(ARCH)
