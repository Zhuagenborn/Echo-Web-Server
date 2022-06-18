add_library(http)

set(HEADER_PATH ${PROJECT_SOURCE_DIR}/include)

target_include_directories(http PUBLIC ${HEADER_PATH})

target_sources(http
    PUBLIC
        ${HEADER_PATH}/http.h
    PRIVATE
        http.cpp
)

target_link_libraries(http
    PRIVATE
        util
)