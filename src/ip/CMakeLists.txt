add_library(ip)

set(HEADER_PATH ${PROJECT_SOURCE_DIR}/include)

target_include_directories(ip PUBLIC ${HEADER_PATH})

target_sources(ip
    PUBLIC
        ${HEADER_PATH}/ip.h
    PRIVATE
        ip.cpp
)

target_link_libraries(ip
    PRIVATE
        util
)