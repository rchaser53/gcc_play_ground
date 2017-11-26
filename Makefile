CC=gcc
CFLAGS=-g -O2 -Wall $(shell sdl-config --cflags)
LDFLAGS=$(shell sdl-config --libs) -lSDL_mixer

PROGRAMS=$(basename $(wildcard *.c))

%: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@ $(LDFLAGS)

all: $(PROGRAMS)

.PHONY: clean
clean:
	rm -f $(PROGRAMS) *.o
	rm -rf build
	mkdir build

new: clean all

hello: clean hello_sdl.cpp
	emcc hello_sdl.cpp -L -lsdl2 -o build/hello.html

mixier: clean sdlwav.c
	$(CC) sdlwav.c -L -lsdl2 -lsdl2_mixer -o build/hello.html