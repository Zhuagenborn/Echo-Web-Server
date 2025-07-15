add_library(test-util)

set(HEADER_PATH ${PROJECT_SOURCE_DIR}/include)

target_include_directories(test-util PUBLIC ${HEADER_PATH})

target_sources(test-util
    PUBLIC
        ${HEADER_PATH}/test_util.h
    PRIVATE
        test_util.cpp
)

target_link_libraries(test-util
    PUBLIC
        util
        log
    PRIVATE
        ${GTEST_LIB}
        ${GMOCK_LIB}
)