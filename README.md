# Heroes Of JinYong
A reimplementation of the DOS game `The legend of Jin Yong Heroes(金庸群侠传)`

# How to build
1. Install cmake and C++ compiler (either GCC/Clang or MSVC)
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
|BUILD_TOOLS|OFF|Build tools(`mergepic`)|
  
# How to use compiled binaries
1. Get original game files (you can download from [here](https://dos.zczc.cz/games/金庸群侠传/download))
2. Copy compiled `bin/hojy.exe` and maybe other dll/so's to your own game root folder
3. Copy `src/config.toml` to root folder
4. Create a subfolder `data` in game root folder
5. Copy `src/strings.toml` to `data` folder
6. Extract downloaded game into `data` (do not create any 2nd-level subfolder)
7. Run `hojy.exe` and enjoy!

## How to merge Submap and Warfield pictures/textures
1. Add `-DBUILD_TOOL=ON` to `cmake` command and build the whole project, you will get `mergepic` in `bin` folder
2. Run `mergepic` in original game data folder using following commands to generate 4 files: `SDX`, `SMP`, `WDX`, `WMP`:
   1. `mergepic SDX SMP`
   2. `mergepic WDX WMP`
3. Once done, you can remove all `SDX???`, `SMP???`, `SDX???`, `WMP???` files from resource folder

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
