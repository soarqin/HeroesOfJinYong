project(HeroesOfJinYongMain)

file(GLOB CORE_FILES  core/*.cc  core/*.hh)
file(GLOB DATA_FILES  data/*.cc  data/*.hh)
file(GLOB MEM_FILES   mem/*.cc   mem/*.hh)
file(GLOB SCENE_FILES scene/*.cc scene/*.hh)
file(GLOB AUDIO_FILES audio/*.cc audio/*.hh)
file(GLOB UTIL_FILES  util/*.cc  util/*.hh)
add_executable(${PROJECT_NAME} main.cc
    ${CORE_FILES} ${DATA_FILES} ${MEM_FILES} ${SCENE_FILES} ${AUDIO_FILES} ${UTIL_FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin RUNTIME_OUTPUT_NAME hojy)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(${PROJECT_NAME} PRIVATE SDL_MAIN_HANDLED)
target_link_libraries(${PROJECT_NAME} soxr ADLMIDI SDL2_gfx)
