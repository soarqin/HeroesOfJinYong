function(message)
    if(MESSAGE_QUIET)
        if((ARGV0 STREQUAL "FATAL_ERROR")
            OR (ARGV0 STREQUAL "SEND_ERROR"))
            _message(${ARGN})
        endif()
    else()
        _message(${ARGN})
    endif()
endfunction()

if(BUILD_SHARED_LIBS)
    set(DEPS_BUILD_SHARED ON)
    set(DEPS_BUILD_STATIC OFF)
else()
    set(DEPS_BUILD_SHARED OFF)
    set(DEPS_BUILD_STATIC ON)
endif()

set(MESSAGE_QUIET ON)
add_subdirectory(fmt)
set(MESSAGE_QUIET OFF)

if(USE_SOXR)
    set(BUILD_TESTS OFF CACHE BOOL "Build sanity-tests." FORCE)
    set(BUILD_EXAMPLES OFF CACHE BOOL "Build examples." FORCE)
    set(WITH_OPENMP OFF CACHE BOOL "Include OpenMP threading." FORCE)
    set(WITH_LSR_BINDINGS OFF CACHE BOOL "Include a `libsamplerate'-like interface." FORCE)
    set(WITH_DEV_TRACE OFF CACHE BOOL "Enable developer trace capability." FORCE)
    set(WITH_DEV_GPROF OFF CACHE BOOL "Enable developer grpof output." FORCE)
    list (APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/soxr/cmake/Modules)
    set(MESSAGE_QUIET ON)
    add_subdirectory(soxr)
    set(MESSAGE_QUIET OFF)
    target_include_directories(soxr PUBLIC soxr/src)
    if(DEPS_BUILD_SHARED)
        if(WIN32)
            target_compile_definitions(soxr PUBLIC SOXR_DLL)
        endif()
        set_target_properties(soxr PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    endif()
endif()

set(libADLMIDI_STATIC ${DEPS_BUILD_STATIC} CACHE BOOL "Build static library of libADLMIDI" FORCE)
set(libADLMIDI_SHARED ${DEPS_BUILD_SHARED} CACHE BOOL "Build shared library of libADLMIDI" FORCE)
set(USE_OPAL_EMULATOR OFF CACHE BOOL "Use Opal emulator (inaccurate)" FORCE)
set(USE_JAVA_EMULATOR OFF CACHE BOOL "Use JavaOPL emulator" FORCE)
set(MESSAGE_QUIET ON)
add_subdirectory(libADLMIDI)
set(MESSAGE_QUIET OFF)
if(DEPS_BUILD_SHARED)
    set_target_properties(ADLMIDI_shared PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endif()

if(WIN32)
    set(LIBC ON CACHE BOOL "Use the system C library" FORCE)
    set(FORCE_STATIC_VCRT ${USE_STATIC_CRT} CACHE BOOL "Force /MT for static VC runtimes" FORCE)
    set(DIRECTX OFF CACHE BOOL "Use DirectX for Windows video/audio" FORCE)
    set(RENDER_D3D OFF CACHE BOOL "Enable the Direct3D render driver" FORCE)
    set(WASAPI ON CACHE BOOL "Use the windows WASAPI audio driver" FORCE)
    set(SDL_SUBSYSTEMS_TO_DISABLE Power Filesystem Dlopen Sensor Locale)
    foreach(_SUB ${SDL_SUBSYSTEMS_TO_DISABLE})
        string(TOUPPER ${_SUB} _OPT)
        set(SDL_${_OPT} OFF CACHE BOOL "Enable the ${_SUB} subsystem" FORCE)
    endforeach()
    set(MESSAGE_QUIET ON)
    add_subdirectory(SDL2)
    set(MESSAGE_QUIET OFF)
    if(TARGET SDL2)
        set_target_properties(SDL2 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    endif()
endif()

add_subdirectory(SDL2_gfx)

if(NOT USE_SOXR)
    add_library(zita-resampler STATIC
        zita-resampler/source/resampler-table.cc
        zita-resampler/source/vresampler.cc)
    target_include_directories(zita-resampler PUBLIC zita-resampler/source)
endif()