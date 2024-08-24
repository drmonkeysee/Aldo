# Aldo

A [MOS Technology 6502](https://en.wikipedia.org/wiki/MOS_Technology_6502) emulator written in C, targeting the C17 standard and using (most) of the guidelines of Modern C.

The current milestone is to build a [NES](https://en.wikipedia.org/wiki/Nintendo_Entertainment_System) emulator, but the long-term goal is for Aldo to be a "fantasy" 6502 computer (similar to [PICO-8's](https://www.lexaloffle.com/pico-8.php) "fantasy console" concept), as well as being able to emulate other 6502 systems beyond the NES, such as the [Commodore 64](https://en.wikipedia.org/wiki/Commodore_64) or [Apple II](https://en.wikipedia.org/wiki/Apple_II).

## Build

At some point I may look into replacing this with [CMake](https://cmake.org) but at the moment the build is pretty simple.

### macOS CLI

The Homebrew and ncurses steps get you the latest version of ncurses and are optional. The OS-shipped version works just as well and will be used automatically if the brew version is not installed.

1. Install Xcode
2. Install [Homebrew](https://brew.sh)
3. `brew install ncurses`
4. `make run` to print usage
5. `make run FILE=<file>` to load a program ROM

### macOS GUI

1. Install Xcode
2. Run `make ext`
3. Open ext/ folder
4. Mount SDL2 dmg file
5. Copy SDL2.framework to ext/ folder
6. Open mac/Aldo.xcodeproj
7. Build and Run "Aldo" target

The GUI app will include a build of the CLI binary in **Aldo.app/Contents/MacOS/Cli**. Unlike the make target, this build will always link to the OS ncurses libraries. In addition, the GUI and CLI executables depend on a shared library embedded in the app bundle rather than statically linking everything. This means the linking and runtime characteristics of the CLI differ somewhat from the make build. Specifically, the bundled CLI binary cannot be moved to another folder or it won't find **libaldo** at load time. If you want a friendlier path for the CLI you can create a soft-link to it in your location of choice, for example:

```sh
ln -s /Applications/Aldo.app/Contents/MacOS/Cli/aldoc /usr/local/bin/aldoc
```

### Debian/Ubuntu CLI

1. Run `locale` to verify your terminal is using a UTF-8 locale (`C.UTF-8` will work)
2. `[sudo] apt update`
3. `[sudo] apt install -y build-essential git libncurses-dev`
4. `make run` to print usage
5. `make run FILE=<file>` to load a program ROM

### Debian/Ubuntu GUI

This build is best done with GCC-12+ as earlier versions have some bugs around how C++ concepts are applied. Follow steps 1-3 of the CLI build above. Then:

4. `[sudo] apt install -y curl libsdl2-dev`
5. Run `make ext`
6. `make debug-gui`

This will _build_ a binary but the platform layer is not yet implemented, so running it will print a diagnostic and exit immediately. At the moment this build is only good for cross-checking compilation between Clang and GCC.

## Test

The Makefile also provides a verification target.

### macOS

1. Follow the [CinyTest build/install instructions](https://github.com/drmonkeysee/CinyTest#build-cinytest)
2. Install [Python3](https://www.python.org) (in order of recommended options)
	- manage installs via [pyenv](https://github.com/pyenv/pyenv) `brew install pyenv` and install any Python version >= 3.8
	- OR directly `brew install Python@3.8` (any version >= 3.8)
	- OR install from the official site linked above
3. `make check`

### Debian/Ubuntu

1. Follow the [CinyTest build/install instructions](https://github.com/drmonkeysee/CinyTest#build-cinytest)
2. Install additional dependencies `[sudo] apt install -y bsdextrautils curl python3`
3. `make check`

The verification target is made up of these test targets:

- `make test`: Aldo unit tests, written using [CinyTest](https://github.com/drmonkeysee/CinyTest)
- `make nestest nesdiff`: kevtris's [nestest CPU tests](https://wiki.nesdev.org/w/index.php?title=Emulator_tests)
- `make bcdtest`: Bruce Clark's [Binary-coded Decimal tests](http://6502.org/tutorials/decimal_mode.html); additional details in [BCDTEST.md](test/BCDTEST.md)

Additionally, the macOS Xcode project's **Dev** target can run the Aldo unit tests. This is equivalent to the `make test` target.

## External Dependencies

Dependencies needed to build and run Aldo components.

### CLI/Test

- [ncurses](https://invisible-island.net/ncurses/man/)
- [Python3](https://www.python.org)
- [CinyTest](https://github.com/drmonkeysee/CinyTest)

### GUI

- [SDL2](https://www.libsdl.org) (Simple Directmedia Layer)
- [Dear ImGui](https://github.com/ocornut/imgui)

## Resources

- [Nesdev Wiki](https://www.nesdev.org/wiki/Nesdev_Wiki)
- [One Lone Coder](https://www.youtube.com/c/javidx9)
- [6502.org](http://6502.org)
- [Programming the 6502, 4th Edition](https://archive.org/details/Programming_the_6502_OCR)
- [Easy 6502](https://skilldrick.github.io/easy6502/)
- [Visual 6502](http://visual6502.org)
- [Ben Eater](https://eater.net)
- [Modern C](https://modernc.gforge.inria.fr)
