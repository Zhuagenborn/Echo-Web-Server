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
        containers/buffer_test.cpp
        io_test.cpp
        config_test.cpp
        containers/block_deque_test.cpp
        log_test.cpp
        containers/heap_timer_test.cpp
        containers/thread_pool_test.cpp
        ip_test.cpp
)

target_link_libraries(test-bundle
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

gtest_discover_tests(test-bundle)