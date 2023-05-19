# gambatte-sdl
Barely working SDL2 frontend for gambatte-speedrun Gameboy emulator, for example to use with a Linux handheld.

Requires a GBC BIOS and a program rom file.

## Usage
To launch a rom, specify the filename as the first command line argument. For example:
`./gambatte-sdl lsdj.gb`

## Building
1. Sync submodules to get the gambatte-speedrun core
2. Build gambatte-core/libgambatte by going to the directory and using `scons`
3. Build the frontend with `./build.sh`


