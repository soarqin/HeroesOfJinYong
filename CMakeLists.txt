cmake_minimum_required(VERSION 3.13)

project(HeroesOfJinYong)

option(BUILD_SHARED_LIBS  "Build shared libraries" ON)
option(USE_STATIC_CRT "Use static C runtime" OFF)
option(USE_FREETYPE "Use freetype instead of stb_truetype" OFF)
option(USE_SOXR "Use soxr instead of zita-resampler(better quality with more cpu use)" OFF)

if(USE_STATIC_CRT)
    if(CMAKE_COMPILER_IS_GNUCXX)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -static-libgcc -static-libstdc++")
    elseif(MSVC)
        add_compile_options(
            $<$<CONFIG:>:/MT>
            $<$<CONFIG:Debug>:/MTd>
            $<$<CONFIG:Release>:/MT>
            $<$<CONFIG:MinSizeRel>:/MT>
            $<$<CONFIG:RelWithDebInfo>:/MT>
        )
    endif()
endif()
if(MSVC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /utf-8")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")
    add_compile_options(
        $<$<CONFIG:Release>:/GL>
        $<$<CONFIG:MinSizeRel>:/GL>
    )
    add_link_options(
        $<$<CONFIG:Release>:/LTCG>
        $<$<CONFIG:MinSizeRel>:/LTCG>
    )
endif()

add_subdirectory(deps)
add_subdirectory(src)
