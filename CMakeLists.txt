cmake_minimum_required(VERSION 3.17)
project(HeroesOfJinYong)

set(CMAKE_CXX_STANDARD 17)

option(USE_STATIC_CRT "Use static C runtime" OFF)
option(USE_FREETYPE "Use freetype instead of stb_truetype" OFF)

if(USE_STATIC_CRT)
    if(CMAKE_COMPILER_IS_GNUCXX)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -static-libgcc -static-libstdc++")
    elseif(MSVC)
        set(CompilerFlags
            CMAKE_CXX_FLAGS
            CMAKE_CXX_FLAGS_DEBUG
            CMAKE_CXX_FLAGS_RELEASE
            CMAKE_CXX_FLAGS_MINSIZEREL
            CMAKE_CXX_FLAGS_RELWITHDEBINFO
            CMAKE_C_FLAGS
            CMAKE_C_FLAGS_DEBUG
            CMAKE_C_FLAGS_RELEASE
            CMAKE_C_FLAGS_MINSIZEREL
            CMAKE_C_FLAGS_RELWITHDEBINFO
            )
        foreach(CompilerFlag ${CompilerFlags})
            if(${CompilerFlag} MATCHES "/MD")
                string(REGEX REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
            endif()
        endforeach()
    endif()
endif()
if(MSVC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /utf-8" CACHE STRING "msvc compiler flags" FORCE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8" CACHE STRING "msvc compiler flags" FORCE)
endif()

add_subdirectory(deps)
add_subdirectory(src)
