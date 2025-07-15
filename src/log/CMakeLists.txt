add_library(log)

set(HEADER_PATH ${PROJECT_SOURCE_DIR}/include)

target_include_directories(log PUBLIC ${HEADER_PATH})

target_sources(log
    PUBLIC
        ${HEADER_PATH}/log.h
    PRIVATE
        log.cpp
        appender.cpp
        field.h
        field.cpp
        config_init.h
        config_init.cpp
)

target_link_libraries(log
    PUBLIC
        util
        block-deque
        config
)

# Private Unit Test
add_executable(log-test)

target_link_libraries(log-test
    PRIVATE
        ${GTEST_LIB}
        log
        $<TARGET_PROPERTY:log,LINK_LIBRARIES>
)

target_include_directories(log-test
    PRIVATE
        $<TARGET_PROPERTY:log,INCLUDE_DIRECTORIES>
)

target_sources(log-test
    PRIVATE
        field_test.cpp
        config_init_test.cpp
)

gtest_discover_tests(log-test)