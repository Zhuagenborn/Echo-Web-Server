set(HEADER_PATH ${PROJECT_SOURCE_DIR}/include/containers)

add_library(block-deque INTERFACE)
target_include_directories(block-deque INTERFACE ${PROJECT_SOURCE_DIR}/include)
target_sources(block-deque INTERFACE ${HEADER_PATH}/block_deque.h)

add_subdirectory(buffer)