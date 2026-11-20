cmake_minimum_required(VERSION 3.16)

project(motion_alarm_tests CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()

add_executable(alarm_tests
    test_main.cpp
    test_motion.cpp
    test_ble.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../source/motion.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../source/ble_alarm.cpp
)

target_include_directories(alarm_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../source)
target_compile_definitions(alarm_tests PRIVATE HOST_TEST)
target_compile_options(alarm_tests PRIVATE -Wall -Wextra)
target_link_libraries(alarm_tests m)

add_test(NAME alarm_tests COMMAND alarm_tests)
