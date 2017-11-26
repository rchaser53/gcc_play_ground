CC=gcc
CFLAGS=-g -O2 -Wall $(shell sdl-config --cflags)
LDFLAGS=$(shell sdl-config --libs) -lSDL_mixer

PROGRAMS=$(basename $(wildcard *.c))

%: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@ $(LDFLAGS)

all: $(PROGRAMS)

clean:
	rm -f $(PROGRAMS) *.o

new: clean all

hello:
	emcc hello_sdl.cpp -L -lsdl2 -o build/hello.html

mixier:
	$(CC) sdlwav.c -L -lsdl2 -o build/hello.html