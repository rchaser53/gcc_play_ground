rm -rf build
mkdir build
emcc hello_sdl.cpp -L -lsdl2 -o build/hello.html