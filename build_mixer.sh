rm -rf build
mkdir build
emcc sdlwav.c -L -lsdl2 -o build/hello.html