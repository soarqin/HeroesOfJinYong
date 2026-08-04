# Heroes Of JinYong
A reimplementation of the DOS game `The legend of Jin Yong Heroes(金庸群侠传)`

# How to build
1. Install cmake and C++ compiler (either GCC 8+, Clang 7+ or MSVC 2019+)
2. Clone the project: `git clone --recurse-submodules https://github.com/soarqin/HeroesOfJinYong`
3. (optional after pull new commits from repository) Update submodules: `git submodule update --init`
4. Use cmake to compile the project (recommended steps):
   1. `mkdir build && cd build`
   2. `cmake ..`
   3. (UNIX OSes/MinGW/CygWIN) `make`
   4. (WIN32) Open project to build

## CMake build options
|Name|Default Value|Description|
|---|---|---|
|BUILD_SHARED_LIBS|ON|Build shared libraries|
|USE_STATIC_CRT|OFF|Use static C runtime|
|USE_FREETYPE|OFF|Use freetype instead of stb_truetype|
|USE_SOXR|OFF|Use soxr instead of zita-resampler(better quality with more cpu use)|
|BUILD_TOOLS|OFF|Build data preparation tools (`makedata` and `mergepic`)|
  
# How to use compiled binaries
1. Get original game files (you can download from [here](https://dos.zczc.cz/games/金庸群侠传/download))
2. Configure CMake with `-DBUILD_TOOLS=ON` and build the project.
3. Run `makedata <original-game-path> <target-path> <font-file>`. The tool creates `data`, copies the required game resources and font, merges the submap and warfield pictures, copies `strings.toml`, and generates `config.toml`.
4. Copy compiled `bin/hojy.exe` and any required DLLs/shared libraries to the target path.
5. Run `hojy.exe` and enjoy!

## How to merge Submap and Warfield pictures/textures
`makedata` performs these merges automatically. The standalone compatibility tool is still available:

1. Add `-DBUILD_TOOLS=ON` to the `cmake` command and build the whole project; `mergepic` is generated in the `bin` folder.
2. Run `mergepic` in original game data folder using following commands to generate 4 files: `SDX`, `SMP`, `WDX`, `WMP`:
   1. `mergepic SDX SMP`
   2. `mergepic WDX WMP`
3. Once done, you can remove all `SDX???`, `SMP???`, `WDX???`, `WMP???` files from the resource folder.

# Documentation
* [Battle logic — mathematical specification](docs/battle-math.md): pure-mathematics description of the battle formulas and AI decision logic (no code/address details)
* [Battle logic — implementation reference](docs/battle-logic.md): battle rules with code locations, memory addresses and modification guide

# License
* This software is licensed under GPLv3, Check [LICENSE](LICENSE) for details.
* External/3rd-party libraries are following their own license, see CREDITS below.

# CREDITS
* Public Domain:
   * [stb_feetype & stb_rect_pack](https://github.com/nothings/stb) ([src/external/stb_*.h](src/external))
* MIT licensed:
   * [toml++](https://github.com/marzer/tomlplusplus) ([src/external/toml.hpp](src/external/toml.hpp))
   * [fmt](https://github.com/fmtlib/fmt) ([deps/fmt](deps/fmt))
* FTL licensed:
   * [FreeType](https://www.freetype.org) (found in OSes)
* GPLv3 licensed:
   * [libADLMIDI](https://github.com/Wohlstand/libADLMIDI) ([deps/libADLMIDI](deps/libADLMIDI))
   * [zita-resampler](https://kokkinizita.linuxaudio.org/linuxaudio/zita-resampler/resampler.html) ([deps/zita-resampler](deps/zita-resampler))
* LGPLv2.1 licensed:
   * [soxr](http://soxr.sourceforge.net/) ([deps/soxr](deps/soxr))
* Zlib licensed:
   * [SDL2](https://www.libsdl.org/) (use [deps/SDL2](deps/SDL2) in Windows for Input Method support, and find system SDL2 in other OSes)
   * [SDL2_gfx](https://sourceforge.net/projects/sdl2gfx/) ([deps/SDL2_gfx](deps/SDL2_gfx))
