cmake_minimum_required(VERSION 3.20.0)

project(echo-web-server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

enable_testing()

find_package(GTest REQUIRED)
set(GTEST_LIBS GTest::gtest GTest::gtest_main)
set(GMOCK_LIBS GTest::gmock GTest::gmock_main)
set(CMAKE_GTEST_DISCOVER_TESTS_DISCOVERY_MODE PRE_TEST)

add_subdirectory(src)
add_subdirectory(tests)