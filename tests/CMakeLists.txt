set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(battle_random_tests battle/random_tests.cc)
target_include_directories(battle_random_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(battle_random_tests PRIVATE hojy_battle)
add_test(NAME battle_random_tests COMMAND battle_random_tests)

add_executable(battle_combat_rules_tests battle/combat_rules_tests.cc)
target_include_directories(battle_combat_rules_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(battle_combat_rules_tests PRIVATE hojy_battle)
add_test(NAME battle_combat_rules_tests COMMAND battle_combat_rules_tests)

add_executable(battle_formula_tests battle/formula_tests.cc)
target_include_directories(battle_formula_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(battle_formula_tests PRIVATE hojy_battle)
add_test(NAME battle_formula_tests COMMAND battle_formula_tests)

add_executable(battle_movement_tests battle/movement_tests.cc)
target_include_directories(battle_movement_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(battle_movement_tests PRIVATE hojy_battle)
add_test(NAME battle_movement_tests COMMAND battle_movement_tests)

add_executable(battle_attack_area_tests battle/attack_area_tests.cc)
target_include_directories(battle_attack_area_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(battle_attack_area_tests PRIVATE hojy_battle)
add_test(NAME battle_attack_area_tests COMMAND battle_attack_area_tests)

add_executable(battle_ai_policy_tests battle/ai_policy_tests.cc)
target_include_directories(battle_ai_policy_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(battle_ai_policy_tests PRIVATE hojy_battle)
add_test(NAME battle_ai_policy_tests COMMAND battle_ai_policy_tests)

add_executable(battle_ai_tests battle/ai_tests.cc)
target_include_directories(battle_ai_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(battle_ai_tests PRIVATE hojy_battle)
add_test(NAME battle_ai_tests COMMAND battle_ai_tests)

add_executable(battle_ai_strategy_tests battle/ai_strategy_tests.cc)
target_include_directories(battle_ai_strategy_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(battle_ai_strategy_tests PRIVATE hojy_battle)
add_test(NAME battle_ai_strategy_tests COMMAND battle_ai_strategy_tests)

add_executable(battle_resource_items_tests battle/resource_items_tests.cc)
target_sources(battle_resource_items_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src/mem/bag.cc
    ${PROJECT_SOURCE_DIR}/src/mem/item_slots.cc)
target_include_directories(battle_resource_items_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(battle_resource_items_tests PRIVATE hojy_battle)
add_test(NAME battle_resource_items_tests COMMAND battle_resource_items_tests)

add_executable(battle_turn_order_tests battle/turn_order_tests.cc)
target_include_directories(battle_turn_order_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(battle_turn_order_tests PRIVATE hojy_battle)
add_test(NAME battle_turn_order_tests COMMAND battle_turn_order_tests)

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
if(TARGET SDL2::SDL2)
add_custom_command(TARGET audio_channel_tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:SDL::SDL2>
        $<TARGET_FILE_DIR:audio_channel_tests>)
endif()
add_test(NAME audio_channel_tests COMMAND audio_channel_tests)

add_executable(npc_item_tests
    mem/npc_item_tests.cc
    ${PROJECT_SOURCE_DIR}/src/mem/npcitem.cc
    ${PROJECT_SOURCE_DIR}/src/mem/item_slots.cc)
target_include_directories(npc_item_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(npc_item_tests PROPERTIES CXX_STANDARD 17)
add_test(NAME npc_item_tests COMMAND npc_item_tests)

add_executable(action_contract_tests
    mem/action_contract_tests.cc
    ${PROJECT_SOURCE_DIR}/src/mem/action.cc
    ${PROJECT_SOURCE_DIR}/src/mem/npcitem.cc
    ${PROJECT_SOURCE_DIR}/src/mem/item_slots.cc
    ${PROJECT_SOURCE_DIR}/src/mem/serializable.cc
    ${PROJECT_SOURCE_DIR}/src/util/random.cc)
target_include_directories(action_contract_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(action_contract_tests PRIVATE hojy_battle)
set_target_properties(action_contract_tests PROPERTIES CXX_STANDARD 17)
add_test(NAME action_contract_tests COMMAND action_contract_tests)

add_executable(scene_node_helpers_tests scene/node_helpers_tests.cc)
target_include_directories(scene_node_helpers_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(scene_node_helpers_tests PROPERTIES CXX_STANDARD 17)
add_test(NAME scene_node_helpers_tests COMMAND scene_node_helpers_tests)

add_executable(scene_warfield_load_tests scene/warfield_load_tests.cc)
target_include_directories(scene_warfield_load_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(scene_warfield_load_tests PROPERTIES CXX_STANDARD 17)
add_test(NAME scene_warfield_load_tests COMMAND scene_warfield_load_tests)

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

# IDA Pro 9.4 ships Python extension modules that must be loaded by the
# matching CPython runtime.  Keep the regular Python interpreter as the
# default, while allowing reverse-engineering tests to select the IDA
# compatible runtime and module search path explicitly.
set(HOJY_REVERSE_PYTHON_EXECUTABLE "${Python3_EXECUTABLE}" CACHE FILEPATH
    "Python interpreter used by reverse-engineering tests")
set(HOJY_REVERSE_PYTHONPATH "${PROJECT_SOURCE_DIR}/tools/reverse" CACHE STRING
    "Semicolon-separated PYTHONPATH for reverse-engineering tests")
string(REPLACE ";" "\\;" HOJY_REVERSE_PYTHONPATH_ESCAPED
       "${HOJY_REVERSE_PYTHONPATH}")

add_test(
    NAME reverse_manifest_tests
    COMMAND "${HOJY_REVERSE_PYTHON_EXECUTABLE}" -m unittest
            tests/reverse/test_manifest.py -v)
set_tests_properties(reverse_manifest_tests PROPERTIES
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    ENVIRONMENT "PYTHONPATH=${HOJY_REVERSE_PYTHONPATH_ESCAPED}")

add_test(
    NAME reverse_extract_cli_tests
    COMMAND "${HOJY_REVERSE_PYTHON_EXECUTABLE}" -m unittest
            tests/reverse/test_extract_cli.py -v)
set_tests_properties(reverse_extract_cli_tests PROPERTIES
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    ENVIRONMENT "PYTHONPATH=${HOJY_REVERSE_PYTHONPATH_ESCAPED}")

add_test(
    NAME reverse_ida_backend_tests
    COMMAND "${HOJY_REVERSE_PYTHON_EXECUTABLE}" -m unittest
            tests/reverse/test_ida_backend.py -v)
set_tests_properties(reverse_ida_backend_tests PROPERTIES
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    ENVIRONMENT "PYTHONPATH=${HOJY_REVERSE_PYTHONPATH_ESCAPED}")

add_test(
    NAME reverse_le_backend_tests
    COMMAND "${Python3_EXECUTABLE}" -m unittest
            tests/reverse/test_le_backend.py -v)
set_tests_properties(reverse_le_backend_tests PROPERTIES
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    ENVIRONMENT "PYTHONPATH=${HOJY_REVERSE_PYTHONPATH_ESCAPED}")
