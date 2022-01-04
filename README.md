# Aldo

A NES emulator written in C, targeting the C17 standard and using (most) of the guidelines of Modern C.

## Build

At some point I may look into replacing this with [CMake](https://cmake.org) but at the moment the build is pretty simple.

### macOS

The Homebrew and ncurses steps get you the latest version of ncurses and are optional. The OS-shipped version works just as well and will be used automatically if the brew version is not installed.

1. Install Xcode
2. Install [Homebrew](https://brew.sh)
3. `brew install ncurses`
4. `make run` to print usage
5. `make run FILE=<file>` to load a program ROM

### Debian/Ubuntu

1. Run `locale` to verify your terminal is using a UTF-8 locale (`C.UTF-8` will work)
2. `[sudo] apt-get update`
3. `[sudo] apt-get install -y build-essential git libncurses-dev` (`libncursesw5-dev` on older OS versions)
4. `make run` to print usage
5. `make run FILE=<file>` to load a program ROM

## Test

The Makefile also provides a unit-test build target.

1. Follow the [CinyTest build/install instructions](https://github.com/drmonkeysee/CinyTest#build-cinytest)
2. `make check`

## Resources

- [Nesdev Wiki](https://wiki.nesdev.org/w/index.php?title=Nesdev_Wiki)
- [One Lone Coder](https://www.youtube.com/c/javidx9)
- [6502.org](http://6502.org)
- [Programming the 6502, 4th Edition](https://archive.org/details/Programming_the_6502_OCR)
- [Easy 6502](https://skilldrick.github.io/easy6502/)
- [Visual 6502](http://visual6502.org)
- [Ben Eater](https://eater.net)
- [Modern C](https://modernc.gforge.inria.fr)
