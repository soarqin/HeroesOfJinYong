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
    if(CMAKE_COMPILER_IS_GNUCXX)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -static-libgcc -static-libstdc++")
        set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -flto")
        set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -flto")
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -flto")
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
            string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
            set(${CompilerFlag} "${${CompilerFlag}}" CACHE STRING "msvc compiler flags" FORCE)
        endforeach()
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
    add_compile_options(/EHsc)
    add_compile_definitions(NOMINMAX)
endif()
add_compile_options(
    $<$<AND:$<NOT:$<COMPILE_LANGUAGE:ASM_NASM>>,$<OR:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<OR:$<CONFIG:Release>,$<CONFIG:MinSizeRel>>,$<NOT:$<BOOL:$<TARGET_PROPERTY:HOJY_DISABLE_MSVC_LTO>>>>:/GL>
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
