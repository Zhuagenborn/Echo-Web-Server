add_executable(integrated-tests)

target_include_directories(integrated-tests
    PRIVATE
        ${PROJECT_SOURCE_DIR}/tests
)

target_link_libraries(integrated-tests
    PRIVATE
        ${GTEST_LIBS}
        ${GMOCK_LIBS}
)

target_sources(integrated-tests
    PRIVATE
        util_test.cpp
        containers/buffer_test.cpp
        io_test.cpp
)

target_link_libraries(integrated-tests
    PRIVATE
        util
        test-util
        buffer
        io
)

gtest_discover_tests(integrated-tests)