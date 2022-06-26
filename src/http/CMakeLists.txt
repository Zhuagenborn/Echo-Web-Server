add_library(http)

set(HEADER_PATH ${PROJECT_SOURCE_DIR}/include)

target_include_directories(http PUBLIC ${HEADER_PATH})

target_sources(http
    PUBLIC
        ${HEADER_PATH}/http.h
    PRIVATE
        http.cpp
        request.h
        request.cpp
        response.h
        response.cpp
)

target_link_libraries(http
    INTERFACE
        ip
    PUBLIC
        util
        buffer
    PRIVATE
        io
)

# Private Unit Test
add_executable(http-test)

target_link_libraries(http-test
    PRIVATE
        ${GTEST_LIBS}
        test-util
        http
        $<TARGET_PROPERTY:http,LINK_LIBRARIES>
)

target_include_directories(http-test
    PRIVATE
        $<TARGET_PROPERTY:http,INCLUDE_DIRECTORIES>
)

target_sources(http-test
    PRIVATE
        request_test.cpp
        response_test.cpp
)

gtest_discover_tests(http-test)