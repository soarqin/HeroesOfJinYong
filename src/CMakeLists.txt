project(HeroesOfJinYongMain CXX)

file(GLOB CORE_FILES  core/*.cc  core/*.hh)
file(GLOB CONTENT_FILES CONFIGURE_DEPENDS content/*.cc content/*.hh)
file(GLOB WORLD_FILES CONFIGURE_DEPENDS world/*.cc world/*.hh)
file(GLOB SCENE_FILES CONFIGURE_DEPENDS scene/*.cc scene/*.hh)
file(GLOB AUDIO_FILES audio/*.cc audio/*.hh)
file(GLOB UTIL_FILES  util/*.cc  util/*.hh)
file(GLOB APP_FILES CONFIGURE_DEPENDS app/*.cc app/*.hh)
file(GLOB EVENT_FILES CONFIGURE_DEPENDS event/*.cc event/*.hh)

# Stable domain targets are introduced before implementation files migrate.
add_library(hojy_content STATIC ${CONTENT_FILES})
set_target_properties(hojy_content PROPERTIES CXX_STANDARD 17)
target_include_directories(hojy_content PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_library(hojy_world STATIC ${WORLD_FILES})
set_target_properties(hojy_world PROPERTIES CXX_STANDARD 17)
target_link_libraries(hojy_world PUBLIC hojy_content)
target_include_directories(hojy_world PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_library(hojy_event STATIC ${EVENT_FILES})
set_target_properties(hojy_event PROPERTIES CXX_STANDARD 17)
target_link_libraries(hojy_event PUBLIC hojy_world)
target_include_directories(hojy_event PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB BATTLE_FILES CONFIGURE_DEPENDS battle/*.cc battle/*.hh)

add_library(hojy_battle STATIC ${BATTLE_FILES})
set_target_properties(hojy_battle PROPERTIES CXX_STANDARD 17)
target_include_directories(hojy_battle PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_library(hojy_scene STATIC ${SCENE_FILES})
set_target_properties(hojy_scene PROPERTIES CXX_STANDARD 17)
target_link_libraries(hojy_scene PUBLIC
    hojy_event hojy_battle hojy_world hojy_content SDL2_gfx)
target_include_directories(hojy_scene PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_library(hojy_app STATIC ${APP_FILES})
set_target_properties(hojy_app PROPERTIES CXX_STANDARD 17)
target_link_libraries(hojy_app PUBLIC hojy_scene SDL2::SDL2)
target_include_directories(hojy_app PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

find_package(Git)
set(VERSION_UPDATE_FROM_GIT ON)
include(GetVersionFromGitTag.cmake)

add_executable(${PROJECT_NAME} WIN32 main.cc
    ${CORE_FILES} ${AUDIO_FILES} ${UTIL_FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 17
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    RUNTIME_OUTPUT_NAME hojy)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(${PROJECT_NAME} PRIVATE SDL_MAIN_HANDLED HOJY_VERSION="${${PROJECT_NAME}_VERSION_STRING_FULL}")
if(USE_FREETYPE)
    find_package(Freetype REQUIRED)
    target_include_directories(hojy_scene PRIVATE ${FREETYPE_INCLUDE_DIRS})
    target_compile_definitions(hojy_scene PRIVATE USE_FREETYPE)
    target_link_libraries(hojy_scene PRIVATE ${FREETYPE_LIBRARIES})
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
target_link_libraries(${PROJECT_NAME}
    hojy_app hojy_scene hojy_event hojy_world hojy_content
    ADLMIDI SDL2_gfx fmt::fmt)
if(CMAKE_COMPILER_IS_GNUCXX)
    target_link_libraries(${PROJECT_NAME} stdc++fs)
endif()
if(BUILD_TOOLS)
    add_executable(mergepic tools/mergepic.cc util/file.cc util/file.hh)
    set_target_properties(mergepic PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    target_include_directories(mergepic PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
endif()
