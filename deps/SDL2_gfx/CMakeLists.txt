project(SDL2_gfx)

find_package(SDL2 REQUIRED)

option(SDL2_GFX_USE_MMX "Use MMX for SDL2_gfx" ON)

add_library(SDL2_gfx STATIC
    SDL2_framerate.c SDL2_framerate.h
    SDL2_gfxPrimitives.c SDL2_gfxPrimitives.h SDL2_gfxPrimitives_font.h
    SDL2_imageFilter.c SDL2_imageFilter.h
    SDL2_rotozoom.c SDL2_rotozoom.h)

target_include_directories(SDL2_gfx PUBLIC ${PROJECT_SOURCE_DIR})
if(TARGET SDL2::SDL2-static)
    target_link_libraries(SDL2_gfx PUBLIC SDL2::SDL2-static)
elseif(TARGET SDL2::SDL2)
    target_link_libraries(SDL2_gfx PUBLIC SDL2::SDL2)
else()
    target_include_directories(SDL2_gfx PUBLIC ${SDL2_INCLUDE_DIRS})
    target_link_libraries(SDL2_gfx PUBLIC ${SDL2_LIBRARIES})
endif()

if(SDL2_GFX_USE_MMX)
    target_compile_definitions(SDL2_gfx PRIVATE USE_MMX)
    if(CMAKE_SYSTEM_PROCESSOR STREQUAL x86 OR CMAKE_SYSTEM_PROCESSOR STREQUAL x64
        OR CMAKE_SYSTEM_PROCESSOR STREQUAL x86_64 OR CMAKE_SYSTEM_PROCESSOR STREQUAL amd64
        OR CMAKE_SYSTEM_PROCESSOR STREQUAL i686)
        if(NOT MSVC AND "${CMAKE_SIZEOF_VOID_P}" STREQUAL "4")
            target_link_options(SDL2_gfx PRIVATE -mmmx)
        endif()
    endif()
endif()
