add_executable(test-bundle)

target_include_directories(test-bundle
    PRIVATE
        ${PROJECT_SOURCE_DIR}/tests
)

target_link_libraries(test-bundle
    PRIVATE
        ${GTEST_LIBS}
        ${GMOCK_LIBS}
)

target_sources(test-bundle
    PRIVATE
        util_test.cpp
)

target_link_libraries(test-bundle
    PRIVATE
        util
)

gtest_discover_tests(test-bundle)