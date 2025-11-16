cmake_minimum_required(VERSION 3.14)
project(ptx_oven_controller_tests)

# GoogleTest requires at least C++14
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/tests/stubs
    ${CMAKE_SOURCE_DIR}/tests/mocks
)

# Source files for the oven control module
set(OVEN_SOURCES
    ptx_oven_config.cpp
    ptx_sensor_filter.cpp
    ptx_actuator.cpp
    ptx_oven_control.cpp
)

# Mock files
set(MOCK_SOURCES
    tests/mocks/mock_api.cpp
    tests/mocks/mock_logging.cpp
)

# Create test executable
add_executable(
    oven_control_test
    tests/test_oven_control_gtest.cpp
    ${OVEN_SOURCES}
    ${MOCK_SOURCES}
)

target_link_libraries(
    oven_control_test
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(oven_control_test)
