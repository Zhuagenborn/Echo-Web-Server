add_library(buffer)

set(HEADER_PATH ${PROJECT_SOURCE_DIR}/include/containers)

target_include_directories(buffer PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_include_directories(buffer PRIVATE ${HEADER_PATH})

target_sources(buffer
    PUBLIC
        ${HEADER_PATH}/buffer.h
    PRIVATE
        buffer.cpp
)

target_link_libraries(buffer
    PRIVATE
        io
)