cmake_minimum_required(VERSION 3.13)

project(HeroesOfJinYong)

include(CTest)

if(WIN32)
    set(DEFAULT_BUILD_SHARED_LIBS OFF)
    set(DEFAULT_USE_STATIC_CRT ON)
else()
    set(DEFAULT_BUILD_SHARED_LIBS ON)
    set(DEFAULT_USE_STATIC_CRT OFF)
endif()
option(BUILD_SHARED_LIBS  "Build shared libraries" ${DEFAULT_BUILD_SHARED_LIBS})
option(USE_STATIC_CRT "Use static C runtime" ${DEFAULT_USE_STATIC_CRT})
option(USE_FREETYPE "Use freetype instead of stb_truetype" OFF)
option(USE_SOXR "Use soxr instead of zita-resampler(better quality with more cpu use)" OFF)

if(USE_STATIC_CRT)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    add_link_options(
        $<$<AND:$<NOT:$<COMPILE_LANGUAGE:ASM_NASM>>,$<OR:$<C_COMPILER_ID:GNU>,$<C_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>>:-static>
        $<$<AND:$<NOT:$<COMPILE_LANGUAGE:ASM_NASM>>,$<C_COMPILER_ID:GNU>>:-static-libgcc>
        $<$<AND:$<NOT:$<COMPILE_LANGUAGE:ASM_NASM>>,$<CXX_COMPILER_ID:GNU>>:-static-libstdc++>
    )
endif()
if(MSVC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /utf-8")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")
endif()
add_compile_options(
    $<$<AND:$<NOT:$<COMPILE_LANGUAGE:ASM_NASM>>,$<OR:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<OR:$<CONFIG:Release>,$<CONFIG:MinSizeRel>>>:/GL>
    $<$<AND:$<NOT:$<COMPILE_LANGUAGE:ASM_NASM>>,$<OR:$<C_COMPILER_ID:GNU>,$<C_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>,$<OR:$<CONFIG:Release>,$<CONFIG:MinSizeRel>>>:-flto>
)
add_link_options(
    $<$<AND:$<NOT:$<COMPILE_LANGUAGE:ASM_NASM>>,$<C_COMPILER_ID:MSVC>,$<OR:$<CONFIG:Release>,$<CONFIG:MinSizeRel>>>:/LTCG>
    $<$<AND:$<NOT:$<COMPILE_LANGUAGE:ASM_NASM>>,$<OR:$<C_COMPILER_ID:GNU>,$<C_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>,$<OR:$<CONFIG:Release>,$<CONFIG:MinSizeRel>>>:-s\;-flto>
)

add_subdirectory(deps)
add_subdirectory(src)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
