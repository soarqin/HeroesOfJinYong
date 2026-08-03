add_executable(battle_random_tests battle/random_tests.cc)
target_include_directories(battle_random_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(battle_random_tests PRIVATE hojy_battle)
add_test(NAME battle_random_tests COMMAND battle_random_tests)

add_executable(battle_formula_tests battle/formula_tests.cc)
target_include_directories(battle_formula_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(battle_formula_tests PRIVATE hojy_battle)
add_test(NAME battle_formula_tests COMMAND battle_formula_tests)

add_executable(battle_ai_tests battle/ai_tests.cc)
target_include_directories(battle_ai_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(battle_ai_tests PRIVATE hojy_battle)
add_test(NAME battle_ai_tests COMMAND battle_ai_tests)

add_executable(battle_turn_order_tests battle/turn_order_tests.cc)
target_include_directories(battle_turn_order_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(battle_turn_order_tests PRIVATE hojy_battle)
add_test(NAME battle_turn_order_tests COMMAND battle_turn_order_tests)

add_executable(npc_item_tests
    mem/npc_item_tests.cc
    ${PROJECT_SOURCE_DIR}/src/mem/npcitem.cc)
target_include_directories(npc_item_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(npc_item_tests PROPERTIES CXX_STANDARD 17)
add_test(NAME npc_item_tests COMMAND npc_item_tests)

add_executable(persistence_tests
    data/persistence_tests.cc
    data/config_stub.cc
    ${PROJECT_SOURCE_DIR}/src/data/event.cc
    ${PROJECT_SOURCE_DIR}/src/data/grpdata.cc
    ${PROJECT_SOURCE_DIR}/src/data/warfielddata.cc
    ${PROJECT_SOURCE_DIR}/src/mem/bag.cc
    ${PROJECT_SOURCE_DIR}/src/mem/savedata.cc
    ${PROJECT_SOURCE_DIR}/src/mem/serializable.cc
    ${PROJECT_SOURCE_DIR}/src/mem/strings.cc
    ${PROJECT_SOURCE_DIR}/src/util/conv.cc
    ${PROJECT_SOURCE_DIR}/src/util/file.cc)
target_include_directories(persistence_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(persistence_tests PROPERTIES CXX_STANDARD 17)
if(CMAKE_COMPILER_IS_GNUCXX)
    target_link_libraries(persistence_tests PRIVATE stdc++fs)
endif()
add_test(NAME persistence_tests COMMAND persistence_tests)

add_executable(audio_channel_tests
    audio/channel_tests.cc
    ${PROJECT_SOURCE_DIR}/src/audio/channel.cc
    ${PROJECT_SOURCE_DIR}/src/audio/channelwav.cc
    ${PROJECT_SOURCE_DIR}/src/util/file.cc)
target_include_directories(audio_channel_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(audio_channel_tests PRIVATE SDL_MAIN_HANDLED)
target_link_libraries(audio_channel_tests PRIVATE SDL2)
set_target_properties(audio_channel_tests PROPERTIES CXX_STANDARD 17)
add_custom_command(TARGET audio_channel_tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:SDL2>
        $<TARGET_FILE_DIR:audio_channel_tests>)
add_test(NAME audio_channel_tests COMMAND audio_channel_tests)

set(STARTUP_EMPTY_DIR ${CMAKE_CURRENT_BINARY_DIR}/startup-empty)
file(MAKE_DIRECTORY ${STARTUP_EMPTY_DIR})
add_test(
    NAME startup_missing_config_test
    COMMAND ${CMAKE_COMMAND}
        -DEXECUTABLE=$<TARGET_FILE:HeroesOfJinYongMain>
        -DWORKING_DIRECTORY=${STARTUP_EMPTY_DIR}
        -DEXPECTED_EXIT=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/startup/expect_exit.cmake)

find_package(Python3 COMPONENTS Interpreter REQUIRED)
add_test(
    NAME reverse_manifest_tests
    COMMAND ${Python3_EXECUTABLE} -m unittest tests/reverse/test_manifest.py -v)
set_tests_properties(reverse_manifest_tests PROPERTIES
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    ENVIRONMENT "PYTHONPATH=${PROJECT_SOURCE_DIR}/tools/reverse")

add_test(
    NAME reverse_extract_cli_tests
    COMMAND ${Python3_EXECUTABLE} -m unittest tests/reverse/test_extract_cli.py -v)
set_tests_properties(reverse_extract_cli_tests PROPERTIES
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    ENVIRONMENT "PYTHONPATH=${PROJECT_SOURCE_DIR}/tools/reverse")

add_test(
    NAME reverse_ida_backend_tests
    COMMAND ${Python3_EXECUTABLE} -m unittest tests/reverse/test_ida_backend.py -v)
set_tests_properties(reverse_ida_backend_tests PROPERTIES
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    ENVIRONMENT "PYTHONPATH=${PROJECT_SOURCE_DIR}/tools/reverse")

add_test(
    NAME reverse_le_backend_tests
    COMMAND ${Python3_EXECUTABLE} -m unittest tests/reverse/test_le_backend.py -v)
set_tests_properties(reverse_le_backend_tests PROPERTIES
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    ENVIRONMENT "PYTHONPATH=${PROJECT_SOURCE_DIR}/tools/reverse")
