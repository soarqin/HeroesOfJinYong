add_executable(battle_random_tests battle/random_tests.cc)
target_include_directories(battle_random_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(battle_random_tests PRIVATE hojy_battle)
add_test(NAME battle_random_tests COMMAND battle_random_tests)
