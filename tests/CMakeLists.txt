set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(battle_random_tests
    battle/random_tests.cc
    ${PROJECT_SOURCE_DIR}/src/util/random.cc)
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
    ${PROJECT_SOURCE_DIR}/src/world/bag.cc
    ${PROJECT_SOURCE_DIR}/src/world/item_slots.cc)
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

add_executable(battle_participant_tests battle/battle_participant_tests.cc)
target_include_directories(battle_participant_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(battle_participant_tests PRIVATE hojy_battle)
add_test(NAME battle_participant_tests COMMAND battle_participant_tests)

add_executable(battle_engine_tests battle/engine_tests.cc)
target_include_directories(battle_engine_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(battle_engine_tests PRIVATE hojy_battle)
set_target_properties(battle_engine_tests PROPERTIES CXX_STANDARD 17)
add_test(NAME battle_engine_tests COMMAND battle_engine_tests)

add_executable(persistence_tests
    content/persistence_tests.cc
    content/config_stub.cc
    ${PROJECT_SOURCE_DIR}/src/util/conv.cc
    ${PROJECT_SOURCE_DIR}/src/util/file.cc)
target_include_directories(persistence_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(persistence_tests PRIVATE hojy_world hojy_content)
set_target_properties(persistence_tests PROPERTIES CXX_STANDARD 17)
if(CMAKE_COMPILER_IS_GNUCXX)
    target_link_libraries(persistence_tests PRIVATE stdc++fs)
endif()
add_test(NAME persistence_tests COMMAND persistence_tests)

add_executable(binary_io_tests
    content/binary_io_tests.cc)
target_include_directories(binary_io_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(binary_io_tests PRIVATE hojy_content)
set_target_properties(binary_io_tests PROPERTIES CXX_STANDARD 17)
if(CMAKE_COMPILER_IS_GNUCXX)
    target_link_libraries(binary_io_tests PRIVATE stdc++fs)
endif()
add_test(NAME binary_io_tests COMMAND binary_io_tests)

add_executable(content_static_bundle_tests
    content/static_bundle_tests.cc)
target_include_directories(content_static_bundle_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(content_static_bundle_tests PRIVATE hojy_content)
set_target_properties(content_static_bundle_tests PROPERTIES CXX_STANDARD 17)
add_test(NAME content_static_bundle_tests COMMAND content_static_bundle_tests)

add_executable(world_state_tests
    world/world_state_tests.cc)
target_include_directories(world_state_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(world_state_tests PRIVATE hojy_world)
set_target_properties(world_state_tests PROPERTIES CXX_STANDARD 17)
add_test(NAME world_state_tests COMMAND world_state_tests)

add_executable(world_item_transaction_tests
    world/item_transaction_tests.cc
    content/config_stub.cc
    ${PROJECT_SOURCE_DIR}/src/util/conv.cc
    ${PROJECT_SOURCE_DIR}/src/util/file.cc
    ${PROJECT_SOURCE_DIR}/src/util/random.cc)
target_include_directories(world_item_transaction_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(world_item_transaction_tests PRIVATE hojy_world hojy_battle)
set_target_properties(world_item_transaction_tests PROPERTIES CXX_STANDARD 17)
add_test(NAME world_item_transaction_tests COMMAND world_item_transaction_tests)

add_executable(world_new_game_transaction_tests
    world/new_game_transaction_tests.cc
    content/config_stub.cc
    ${PROJECT_SOURCE_DIR}/src/util/conv.cc
    ${PROJECT_SOURCE_DIR}/src/util/file.cc
    ${PROJECT_SOURCE_DIR}/src/util/random.cc)
target_include_directories(world_new_game_transaction_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(world_new_game_transaction_tests PRIVATE
    hojy_world hojy_battle)
set_target_properties(world_new_game_transaction_tests PROPERTIES CXX_STANDARD 17)
add_test(NAME world_new_game_transaction_tests
    COMMAND world_new_game_transaction_tests)

add_executable(audio_channel_tests
    audio/channel_tests.cc
    ${PROJECT_SOURCE_DIR}/src/audio/channel.cc
    ${PROJECT_SOURCE_DIR}/src/audio/channelwav.cc
    ${PROJECT_SOURCE_DIR}/src/util/file.cc)
target_include_directories(audio_channel_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(audio_channel_tests PRIVATE SDL_MAIN_HANDLED)
target_link_libraries(audio_channel_tests PRIVATE SDL2::SDL2)
set_target_properties(audio_channel_tests PROPERTIES CXX_STANDARD 17)
if(TARGET SDL2::SDL2)
    add_custom_command(TARGET audio_channel_tests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SDL2::SDL2>
            $<TARGET_FILE_DIR:audio_channel_tests>)
endif()
add_test(NAME audio_channel_tests COMMAND audio_channel_tests)

add_executable(npc_item_tests
    world/npc_item_tests.cc
    ${PROJECT_SOURCE_DIR}/src/world/npcitem.cc
    ${PROJECT_SOURCE_DIR}/src/world/item_slots.cc)
target_include_directories(npc_item_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(npc_item_tests PROPERTIES CXX_STANDARD 17)
add_test(NAME npc_item_tests COMMAND npc_item_tests)

add_executable(action_contract_tests
    battle/action_contract_tests.cc
    ${PROJECT_SOURCE_DIR}/src/world/action.cc
    ${PROJECT_SOURCE_DIR}/src/world/npcitem.cc
    ${PROJECT_SOURCE_DIR}/src/world/item_slots.cc
    ${PROJECT_SOURCE_DIR}/src/world/serializable.cc
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

add_executable(scene_node_lifetime_tests scene/node_lifetime_tests.cc
    ${PROJECT_SOURCE_DIR}/src/scene/node.cc)
target_include_directories(scene_node_lifetime_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(scene_node_lifetime_tests PRIVATE hojy_scene_logic)
set_target_properties(scene_node_lifetime_tests PROPERTIES CXX_STANDARD 17)
add_test(NAME scene_node_lifetime_tests COMMAND scene_node_lifetime_tests)

add_executable(scene_node_phase_tests
    scene/node_phase_tests.cc
    ${PROJECT_SOURCE_DIR}/src/scene/node.cc
    ${PROJECT_SOURCE_DIR}/src/util/file.cc
    ${PROJECT_SOURCE_DIR}/tests/content/config_stub.cc)
target_include_directories(scene_node_phase_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(scene_node_phase_tests PRIVATE hojy_scene)
set_target_properties(scene_node_phase_tests PROPERTIES CXX_STANDARD 17)
add_test(NAME scene_node_phase_tests COMMAND scene_node_phase_tests)

add_executable(scene_command_queue_tests scene/command_queue_tests.cc)
target_include_directories(scene_command_queue_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(scene_command_queue_tests PRIVATE hojy_scene_logic)
set_target_properties(scene_command_queue_tests PROPERTIES CXX_STANDARD 17)
add_test(NAME scene_command_queue_tests COMMAND scene_command_queue_tests)

add_executable(scene_render_phase_tests
    scene/render_phase_tests.cc
    ${PROJECT_SOURCE_DIR}/src/scene/node.cc)
target_include_directories(scene_render_phase_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(scene_render_phase_tests PRIVATE hojy_scene_logic)
set_target_properties(scene_render_phase_tests PROPERTIES CXX_STANDARD 17)
add_test(NAME scene_render_phase_tests COMMAND scene_render_phase_tests)

add_executable(scene_extendednode_input_tests
    scene/extendednode_input_tests.cc
    ${PROJECT_SOURCE_DIR}/src/util/file.cc
    ${PROJECT_SOURCE_DIR}/tests/content/config_stub.cc)
target_include_directories(scene_extendednode_input_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(scene_extendednode_input_tests PRIVATE hojy_scene)
set_target_properties(scene_extendednode_input_tests PROPERTIES CXX_STANDARD 17)
add_test(NAME scene_extendednode_input_tests COMMAND scene_extendednode_input_tests)

add_executable(scene_warfield_input_mode_tests
    scene/warfield_input_mode_tests.cc)
target_include_directories(scene_warfield_input_mode_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(scene_warfield_input_mode_tests PRIVATE hojy_scene_logic)
set_target_properties(scene_warfield_input_mode_tests PROPERTIES CXX_STANDARD 17)
add_test(NAME scene_warfield_input_mode_tests COMMAND scene_warfield_input_mode_tests)

add_executable(scene_title_input_mode_tests
    scene/title_input_mode_tests.cc)
target_include_directories(scene_title_input_mode_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(scene_title_input_mode_tests PRIVATE hojy_scene_logic)
set_target_properties(scene_title_input_mode_tests PROPERTIES CXX_STANDARD 17)
add_test(NAME scene_title_input_mode_tests COMMAND scene_title_input_mode_tests)

add_executable(scene_title_snapshot_tests
    scene/title_snapshot_tests.cc
    content/config_stub.cc
    ${PROJECT_SOURCE_DIR}/src/util/conv.cc
    ${PROJECT_SOURCE_DIR}/src/util/file.cc)
target_include_directories(scene_title_snapshot_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(scene_title_snapshot_tests PRIVATE hojy_scene)
set_target_properties(scene_title_snapshot_tests PROPERTIES CXX_STANDARD 17)
add_test(NAME scene_title_snapshot_tests COMMAND scene_title_snapshot_tests)

add_executable(scene_title_integration_tests
    scene/title_integration_tests.cc
    ${PROJECT_SOURCE_DIR}/src/core/config.cc
    ${PROJECT_SOURCE_DIR}/src/core/resourcemgr.cc
    ${PROJECT_SOURCE_DIR}/src/util/conv.cc
    ${PROJECT_SOURCE_DIR}/src/util/file.cc
    ${PROJECT_SOURCE_DIR}/src/util/math.cc
    ${PROJECT_SOURCE_DIR}/src/util/random.cc)
target_include_directories(scene_title_integration_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(scene_title_integration_tests PRIVATE hojy_scene)
set_target_properties(scene_title_integration_tests PROPERTIES CXX_STANDARD 17)
add_test(NAME scene_title_integration_tests COMMAND scene_title_integration_tests)

add_executable(scene_title_render_layout_tests
    scene/title_render_layout_tests.cc)
target_include_directories(scene_title_render_layout_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(scene_title_render_layout_tests PROPERTIES CXX_STANDARD 17)
add_test(NAME scene_title_render_layout_tests COMMAND scene_title_render_layout_tests)

add_executable(scene_battle_presentation_session_tests
    scene/battle_presentation_session_tests.cc)
target_include_directories(scene_battle_presentation_session_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/tests)
target_link_libraries(scene_battle_presentation_session_tests PRIVATE hojy_scene_logic)
set_target_properties(scene_battle_presentation_session_tests PROPERTIES CXX_STANDARD 17)
add_test(NAME scene_battle_presentation_session_tests
    COMMAND scene_battle_presentation_session_tests)

add_executable(scene_submap_contract_tests
    scene/submap_contract_tests.cc)
target_include_directories(scene_submap_contract_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/tests)
target_link_libraries(scene_submap_contract_tests PRIVATE hojy_scene_logic)
set_target_properties(scene_submap_contract_tests PROPERTIES CXX_STANDARD 17)
add_test(NAME scene_submap_contract_tests COMMAND scene_submap_contract_tests)

add_executable(scene_talk_layout_tests
    scene/talk_layout_tests.cc)
target_include_directories(scene_talk_layout_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/tests)
target_link_libraries(scene_talk_layout_tests PRIVATE hojy_scene_logic)
set_target_properties(scene_talk_layout_tests PROPERTIES CXX_STANDARD 17)
add_test(NAME scene_talk_layout_tests COMMAND scene_talk_layout_tests)

add_executable(scene_font_metrics_tests
    scene/font_metrics_tests.cc)
target_include_directories(scene_font_metrics_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/tests)
set_target_properties(scene_font_metrics_tests PROPERTIES CXX_STANDARD 17)
add_test(NAME scene_font_metrics_tests COMMAND scene_font_metrics_tests)

add_executable(scene_battle_effect_snapshot_tests
    scene/battle_effect_snapshot_tests.cc)
target_include_directories(scene_battle_effect_snapshot_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/tests)
target_link_libraries(scene_battle_effect_snapshot_tests PRIVATE hojy_scene_logic)
set_target_properties(scene_battle_effect_snapshot_tests PROPERTIES CXX_STANDARD 17)
add_test(NAME scene_battle_effect_snapshot_tests
    COMMAND scene_battle_effect_snapshot_tests)

add_executable(scene_fade_timeline_tests scene/fade_timeline_tests.cc)
target_include_directories(scene_fade_timeline_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(scene_fade_timeline_tests PROPERTIES CXX_STANDARD 17)
add_test(NAME scene_fade_timeline_tests COMMAND scene_fade_timeline_tests)

add_executable(scene_warfield_load_tests scene/warfield_load_tests.cc)
target_include_directories(scene_warfield_load_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(scene_warfield_load_tests PROPERTIES CXX_STANDARD 17)
add_test(NAME scene_warfield_load_tests COMMAND scene_warfield_load_tests)

add_executable(scene_color_palette_tests
    scene/color_palette_tests.cc
    ${PROJECT_SOURCE_DIR}/src/scene/colorpalette.cc
    ${PROJECT_SOURCE_DIR}/src/util/file.cc
    ${PROJECT_SOURCE_DIR}/tests/content/config_stub.cc)
target_include_directories(scene_color_palette_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(scene_color_palette_tests PROPERTIES CXX_STANDARD 17)
add_test(NAME scene_color_palette_tests COMMAND scene_color_palette_tests)

add_executable(scene_effect_load_tests
    scene/effect_load_tests.cc
    ${PROJECT_SOURCE_DIR}/src/scene/effect.cc
    ${PROJECT_SOURCE_DIR}/src/util/file.cc
    ${PROJECT_SOURCE_DIR}/tests/content/config_stub.cc)
target_include_directories(scene_effect_load_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(scene_effect_load_tests PRIVATE hojy_content)
set_target_properties(scene_effect_load_tests PROPERTIES CXX_STANDARD 17)
add_test(NAME scene_effect_load_tests COMMAND scene_effect_load_tests)

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

if(TARGET makedata)
    add_test(
        NAME makedata_tests
        COMMAND "${Python3_EXECUTABLE}"
                ${CMAKE_CURRENT_SOURCE_DIR}/tools/makedata_tests.py
                $<TARGET_FILE:makedata>)
    set_tests_properties(makedata_tests PROPERTIES
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
endif()

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

add_test(
    NAME architecture_namespace_scan_tests
    COMMAND "${Python3_EXECUTABLE}" -m unittest
            tests/architecture/namespace_scan_tests.py -v)
set_tests_properties(architecture_namespace_scan_tests PROPERTIES
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    ENVIRONMENT "PYTHONPATH=${PROJECT_SOURCE_DIR}/tools/architecture")

add_test(
    NAME architecture_build_contract_tests
    COMMAND "${Python3_EXECUTABLE}" -m unittest
            tests/architecture/build_contract_tests.py -v)
set_tests_properties(architecture_build_contract_tests PROPERTIES
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    ENVIRONMENT "PYTHONPATH=${PROJECT_SOURCE_DIR}/tools/architecture")

add_test(
    NAME architecture_source_layout_tests
    COMMAND "${Python3_EXECUTABLE}" -m unittest
            tests/architecture/source_layout_tests.py -v)
set_tests_properties(architecture_source_layout_tests PROPERTIES
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})

add_test(
    NAME architecture_render_purity_tests
    COMMAND "${Python3_EXECUTABLE}" -m unittest
            tests/architecture/render_purity_tests.py -v)
set_tests_properties(architecture_render_purity_tests PROPERTIES
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})

add_test(
    NAME architecture_audio_callback_purity_tests
    COMMAND "${Python3_EXECUTABLE}" -m unittest
            tests/architecture/audio_callback_purity_tests.py -v)
set_tests_properties(architecture_audio_callback_purity_tests PROPERTIES
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})

add_test(
    NAME architecture_app_scene_boundary_tests
    COMMAND "${Python3_EXECUTABLE}" -m unittest
            tests/architecture/app_scene_boundary_tests.py -v)
set_tests_properties(architecture_app_scene_boundary_tests PROPERTIES
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})

add_test(
    NAME architecture_warfield_battle_boundary_tests
    COMMAND "${Python3_EXECUTABLE}" -m unittest
            tests/architecture/warfield_battle_boundary_tests.py -v)
set_tests_properties(architecture_warfield_battle_boundary_tests PROPERTIES
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})

add_test(
    NAME architecture_logic_render_boundary_tests
    COMMAND "${Python3_EXECUTABLE}" -m unittest
            tests/architecture/logic_render_boundary_tests.py -v)
set_tests_properties(architecture_logic_render_boundary_tests PROPERTIES
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})

add_test(
    NAME architecture_input_logic_boundary_tests
    COMMAND "${Python3_EXECUTABLE}" -m unittest
            tests/architecture/input_logic_boundary_tests.py -v)
set_tests_properties(architecture_input_logic_boundary_tests PROPERTIES
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})

add_test(
    NAME architecture_scene_command_boundary_tests
    COMMAND "${Python3_EXECUTABLE}" -m unittest
            tests/architecture/scene_command_boundary_tests.py -v)
set_tests_properties(architecture_scene_command_boundary_tests PROPERTIES
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})

add_test(
    NAME architecture_presentation_command_boundary_tests
    COMMAND "${Python3_EXECUTABLE}" -m unittest
            tests/architecture/presentation_command_boundary_tests.py -v)
set_tests_properties(architecture_presentation_command_boundary_tests PROPERTIES
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})

add_test(
    NAME architecture_registration_tests
    COMMAND "${Python3_EXECUTABLE}" -m unittest
            tests/architecture/architecture_registration_tests.py -v)
set_tests_properties(architecture_registration_tests PROPERTIES
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})

add_test(
    NAME architecture_transition_lifecycle_tests
    COMMAND "${Python3_EXECUTABLE}" -m unittest
            tests/architecture/transition_lifecycle_tests.py -v)
set_tests_properties(architecture_transition_lifecycle_tests PROPERTIES
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})

add_test(
    NAME architecture_event_vm_boundary_tests
    COMMAND "${Python3_EXECUTABLE}" -m unittest
            tests/architecture/event_vm_boundary_tests.py -v)
set_tests_properties(architecture_event_vm_boundary_tests PROPERTIES
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})

add_executable(input_scheduler_tests app/input_scheduler_tests.cc)
target_include_directories(input_scheduler_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(input_scheduler_tests PRIVATE hojy_app)
set_target_properties(input_scheduler_tests PROPERTIES CXX_STANDARD 17)
add_test(NAME input_scheduler_tests COMMAND input_scheduler_tests)

add_executable(input_event_contract_tests app/input_event_contract_tests.cc)
target_include_directories(input_event_contract_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(input_event_contract_tests PROPERTIES CXX_STANDARD 17)
add_test(NAME input_event_contract_tests COMMAND input_event_contract_tests)

add_executable(scene_input_port_tests scene/input_port_tests.cc)
target_include_directories(scene_input_port_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(scene_input_port_tests PRIVATE hojy_scene_logic)
set_target_properties(scene_input_port_tests PROPERTIES CXX_STANDARD 17)
add_test(NAME scene_input_port_tests COMMAND scene_input_port_tests)

add_executable(scene_map_input_mode_tests scene/map_input_mode_tests.cc)
target_include_directories(scene_map_input_mode_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(scene_map_input_mode_tests PRIVATE hojy_scene_logic)
set_target_properties(scene_map_input_mode_tests PROPERTIES CXX_STANDARD 17)
add_test(NAME scene_map_input_mode_tests COMMAND scene_map_input_mode_tests)

add_executable(scene_menu_selection_tests scene/menu_selection_tests.cc)
target_include_directories(scene_menu_selection_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(scene_menu_selection_tests PRIVATE hojy_scene_logic)
set_target_properties(scene_menu_selection_tests PROPERTIES CXX_STANDARD 17)
add_test(NAME scene_menu_selection_tests COMMAND scene_menu_selection_tests)

add_executable(scene_item_selection_controller_tests
    scene/item_selection_controller_tests.cc
    ${PROJECT_SOURCE_DIR}/src/scene/item_selection_controller.cc)
target_include_directories(scene_item_selection_controller_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(scene_item_selection_controller_tests PRIVATE hojy_scene_logic)
set_target_properties(scene_item_selection_controller_tests PROPERTIES CXX_STANDARD 17)
add_test(NAME scene_item_selection_controller_tests
    COMMAND scene_item_selection_controller_tests)

add_executable(scene_menu_widget_mode_tests
    scene/menu_widget_mode_tests.cc
    ${PROJECT_SOURCE_DIR}/src/core/config.cc
    ${PROJECT_SOURCE_DIR}/src/core/resourcemgr.cc
    ${PROJECT_SOURCE_DIR}/src/util/conv.cc
    ${PROJECT_SOURCE_DIR}/src/util/file.cc
    ${PROJECT_SOURCE_DIR}/src/util/math.cc
    ${PROJECT_SOURCE_DIR}/src/util/random.cc)
target_include_directories(scene_menu_widget_mode_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(scene_menu_widget_mode_tests PRIVATE hojy_scene)
set_target_properties(scene_menu_widget_mode_tests PROPERTIES CXX_STANDARD 17)
add_test(NAME scene_menu_widget_mode_tests COMMAND scene_menu_widget_mode_tests)

add_executable(scene_rle_validation_tests
    scene/rle_validation_tests.cc)
target_include_directories(scene_rle_validation_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(scene_rle_validation_tests PRIVATE hojy_scene_logic)
set_target_properties(scene_rle_validation_tests PROPERTIES CXX_STANDARD 17)
add_test(NAME scene_rle_validation_tests COMMAND scene_rle_validation_tests)

add_executable(input_repeat_tests app/input_repeat_tests.cc)
target_include_directories(input_repeat_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(input_repeat_tests PRIVATE hojy_app)
set_target_properties(input_repeat_tests PROPERTIES CXX_STANDARD 17)
add_test(NAME input_repeat_tests COMMAND input_repeat_tests)

add_executable(application_contract_tests app/application_contract_tests.cc)
target_include_directories(application_contract_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(application_contract_tests PRIVATE hojy_app)
set_target_properties(application_contract_tests PROPERTIES CXX_STANDARD 17)
add_test(NAME application_contract_tests COMMAND application_contract_tests)

add_executable(rate_scheduler_tests app/rate_scheduler_tests.cc)
target_include_directories(rate_scheduler_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(rate_scheduler_tests PRIVATE hojy_app)
set_target_properties(rate_scheduler_tests PROPERTIES CXX_STANDARD 17)
add_test(NAME rate_scheduler_tests COMMAND rate_scheduler_tests)

add_executable(event_vm_tests event/event_vm_tests.cc)
target_include_directories(event_vm_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(event_vm_tests PRIVATE hojy_event)
set_target_properties(event_vm_tests PROPERTIES CXX_STANDARD 17)
add_test(NAME event_vm_tests COMMAND event_vm_tests)
