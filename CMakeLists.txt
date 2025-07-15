cmake_minimum_required(VERSION 3.20.0)

project(echo-web-server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

option(ECHO_WEB_SERVER_BUILD_TESTS "Build unit tests for the echo web server" OFF)
if(ECHO_WEB_SERVER_BUILD_TESTS)
    find_package(GTest)
    if(GTest_FOUND)
        enable_testing()
        set(GTEST_LIB GTest::gtest GTest::gtest_main)
        set(GMOCK_LIB GTest::gmock GTest::gmock_main)
        set(CMAKE_GTEST_DISCOVER_TESTS_DISCOVERY_MODE PRE_TEST)
        add_subdirectory(tests)
    endif()
endif()

add_subdirectory(src)

file(COPY assets DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
file(COPY config.yaml DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})