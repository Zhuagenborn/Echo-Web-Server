add_library(util)

set(HEADER_PATH ${PROJECT_SOURCE_DIR}/include)

target_include_directories(util PUBLIC ${HEADER_PATH})

target_sources(util
    PUBLIC
        ${HEADER_PATH}/util.h
    PRIVATE
        util.cpp
)

find_package(yaml-cpp REQUIRED)
target_include_directories(util PUBLIC ${YAML_INCLUDE_DIRS})
target_link_libraries(util PUBLIC ${YAML_CPP_LIBRARIES})

find_package(fmt REQUIRED)
target_link_libraries(util PUBLIC fmt::fmt)