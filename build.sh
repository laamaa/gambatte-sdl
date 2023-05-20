#!/bin/sh
echo -- Checking out submodules --
git submodule update --init --recursive
echo -- Building libgambatte --
cd gambatte-core/libgambatte
scons
echo -- Building gambatte-sdl2 --
cd ../..
g++ -o gambatte-sdl2 main.cpp gambatte-core/common/*.cpp gambatte-core/common/resample/src/*.cpp usec.cpp audiosink.cpp input.cpp gambatte-core/libgambatte/libgambatte.a `pkg-config sdl2 --cflags --libs` -I gambatte-core/libgambatte/include/ -I gambatte-core/common/ -lz -Wall -I gambatte-core/ -O2
