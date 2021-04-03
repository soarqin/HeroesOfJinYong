# Heroes Of JinYong
A reimplementation of the DOS game `The legend of Jin Yong Heroes(金庸群侠传)`

# How to build
1. install cmake and C++ compiler (either GCC/Clang or MSVC)
2. for 1st time you need to clone the project: `git clone --recurse-submodules https://github.com/soarqin/HeroesOfJinYong`
3. (optional after pull new commits from repository) update submodules: `git submodule update --init`
4. use cmake to compile the project (recommended steps):
   1. `mkdir build`
   2. `cd build`
   3. `cmake ..`
   4. (UNIX OSes/MinGW/CygWIN) `make`
   4. (WIN32) open project to build
  
# How to use compiled binaries:
1. get original game files (you can download from [here](https://dos.zczc.cz/games/金庸群侠传/download))
2. copy compiled `bin/hojy.exe` and maybe other dll/so's to your own game root folder
3. copy `src/config.toml` to root folder
4. create a subfolder `data` in game root folder
5. copy `src/strings.toml` to `data` folder
6. extrace downloaded game into `data` (do not create any 2nd-level subfolder)
7. run `hojy.exe` and enjoy!

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
