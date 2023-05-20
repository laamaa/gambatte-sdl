# gambatte-sdl2
Barely working SDL2 frontend for gambatte-speedrun Gameboy emulator, for example to use with a Linux handheld.
Based on original GPL-licensed Gambatte SDL and QT frontends.

Requires a GBC BIOS (the program looks for a file called `gbc_bios.bin` in the app directory) and a program rom file.

## Usage
To launch a rom, specify the filename as the first command line argument. For example:
`./gambatte-sdl2 lsdj.gb`

## Building
1. Make sure you have a git executable in your system path and SDL2 and zlib development headers installed
3. Run `./build.sh`

## Controls

### Keyboard controls
* Arrow keys = D-PAD
* D = A button
* S = B button
* X / Space = Start button
* Z / LShift = Select button
* Del = A+B 
* ESC = Quit

### Joypad controls

Joypads are handled by SDL2's Gamecontroller subsystem.
The program is configured to use the controller's D-PAD (or analog) directions and A/B/Back/Start buttons.
It's possible to exit the program by pressing Back, Start and Guide buttons simultaneously.

