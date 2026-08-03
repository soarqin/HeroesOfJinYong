add_executable(battle_random_tests battle/random_tests.cc)
target_include_directories(battle_random_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(battle_random_tests PRIVATE hojy_battle)
add_test(NAME battle_random_tests COMMAND battle_random_tests)

add_executable(battle_turn_order_tests battle/turn_order_tests.cc)
target_include_directories(battle_turn_order_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(battle_turn_order_tests PRIVATE hojy_battle)
add_test(NAME battle_turn_order_tests COMMAND battle_turn_order_tests)

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
