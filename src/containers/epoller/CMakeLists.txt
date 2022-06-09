add_library(epoller)

set(HEADER_PATH ${PROJECT_SOURCE_DIR}/include/containers)

target_include_directories(epoller PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_include_directories(epoller PRIVATE ${HEADER_PATH})

target_sources(epoller
    PUBLIC
        ${HEADER_PATH}/epoller.h
    PRIVATE
        epoller.cpp
)

target_link_libraries(epoller
    PUBLIC
        util
)