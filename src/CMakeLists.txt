project(HeroesOfJinYongMain)

find_package(SDL2 REQUIRED)

file(GLOB CORE_FILES  core/*.cc  core/*.hh)
file(GLOB DATA_FILES  data/*.cc  data/*.hh)
file(GLOB SCENE_FILES scene/*.cc scene/*.hh)
file(GLOB AUDIO_FILES audio/*.cc audio/*.hh)
file(GLOB UTIL_FILES  util/*.cc  util/*.hh)
add_executable(${PROJECT_NAME} main.cc
    ${CORE_FILES} ${DATA_FILES} ${SCENE_FILES} ${AUDIO_FILES} ${UTIL_FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin RUNTIME_OUTPUT_NAME hojy)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(${PROJECT_NAME} PRIVATE SDL_MAIN_HANDLED)
if(TARGET SDL2::SDL2-static)
    target_link_libraries(${PROJECT_NAME} SDL2::SDL2-static)
elseif(TARGET SDL2::SDL2)
    target_link_libraries(${PROJECT_NAME} SDL2::SDL2)
else()
    target_include_directories(${PROJECT_NAME} PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARIES})
endif()
target_link_libraries(${PROJECT_NAME} soxr ADLMIDI)
