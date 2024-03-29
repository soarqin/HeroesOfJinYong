project(HeroesOfJinYongMain CXX)

file(GLOB CORE_FILES  core/*.cc  core/*.hh)
file(GLOB DATA_FILES  data/*.cc  data/*.hh)
file(GLOB MEM_FILES   mem/*.cc   mem/*.hh)
file(GLOB SCENE_FILES scene/*.cc scene/*.hh)
file(GLOB AUDIO_FILES audio/*.cc audio/*.hh)
file(GLOB UTIL_FILES  util/*.cc  util/*.hh)

find_package(Git)
set(VERSION_UPDATE_FROM_GIT ON)
include(GetVersionFromGitTag.cmake)

add_executable(${PROJECT_NAME} WIN32 main.cc
    ${CORE_FILES} ${DATA_FILES} ${MEM_FILES} ${SCENE_FILES} ${AUDIO_FILES} ${UTIL_FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 17
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    RUNTIME_OUTPUT_NAME hojy)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(${PROJECT_NAME} PRIVATE SDL_MAIN_HANDLED HOJY_VERSION="${${PROJECT_NAME}_VERSION_STRING_FULL}")
if(USE_FREETYPE)
    find_package(Freetype REQUIRED)
    target_include_directories(${PROJECT_NAME} PRIVATE ${FREETYPE_INCLUDE_DIRS})
    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_FREETYPE)
    target_link_libraries(${PROJECT_NAME} ${FREETYPE_LIBRARIES})
endif()
if(USE_SOXR)
    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_SOXR)
    target_link_libraries(${PROJECT_NAME} soxr)
else()
    target_link_libraries(${PROJECT_NAME} zita-resampler)
endif()
target_link_libraries(${PROJECT_NAME} ADLMIDI SDL2_gfx fmt::fmt)
if(CMAKE_COMPILER_IS_GNUCXX)
    target_link_libraries(${PROJECT_NAME} stdc++fs)
endif()
if(BUILD_TOOLS)
    add_executable(mergepic tools/mergepic.cc util/file.cc util/file.hh)
    set_target_properties(mergepic PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    target_include_directories(mergepic PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
endif()
