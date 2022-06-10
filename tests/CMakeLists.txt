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
        config_test.cpp
        containers/block_deque_test.cpp
        log_test.cpp
        containers/heap_timer_test.cpp
        containers/thread_pool_test.cpp
        ip_test.cpp
)

target_link_libraries(integrated-tests
    PRIVATE
        util
        test-util
        buffer
        io
        config
        block-deque
        log
        heap-timer
        thread-pool
        ip
)

gtest_discover_tests(integrated-tests)