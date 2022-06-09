add_library(io)

set(HEADER_PATH ${PROJECT_SOURCE_DIR}/include)

target_include_directories(io PUBLIC ${HEADER_PATH})

target_sources(io
    PUBLIC
        ${HEADER_PATH}/io.h
    PRIVATE
        io.cpp
)

target_link_libraries(io
    PUBLIC
        util
    PRIVATE
        buffer
)