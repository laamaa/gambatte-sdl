#!/bin/sh
g++ -o gambatte-sdl main.cpp gambatte-core/common/*.cpp gambatte-core/common/resample/src/*.cpp usec.cpp audiosink.cpp input.cpp libgambatte.a `pkg-config sdl2 --cflags --libs` -I gambatte-core/libgambatte/include/ -I gambatte-core/common/ -lz -Wall -I gambatte-core/ -O2
