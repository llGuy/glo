# Darwin for macos or Linux for linux
OS := $(shell uname -s)

SRC=net.c bitv.c math.c glo.c render.c io.c
CFLAGS=-g
LDFLAGS=-lglfw -lGLEW -lm

ifeq ($(OS),Darwin)
	CFLAGS += -DGLO_MACOS
	LDFLAGS += -framework OpenGL -framework Cocoa 
endif

ifeq ($(OS),Linux)
	CFLAGS += -DGLO_LINUX
	LDFLAGS += -lGL
endif

all: client server
	

client: 
	gcc -o gloc $(CFLAGS) $(SRC) $(LDFLAGS) -DBUILD_CLIENT
server:
	gcc -o glos $(CFLAGS) $(SRC) $(LDFLAGS) 
run:
	./gloc
