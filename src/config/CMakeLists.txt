add_library(config)

set(HEADER_PATH ${PROJECT_SOURCE_DIR}/include)

target_include_directories(config PUBLIC ${HEADER_PATH})

target_sources(config
    PUBLIC
        ${HEADER_PATH}/config.h
    PRIVATE
        config.cpp
)

target_link_libraries(config
    PUBLIC
        util
)