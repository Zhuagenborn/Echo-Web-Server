set(HEADER_PATH ${PROJECT_SOURCE_DIR}/include/containers)

add_library(block-deque INTERFACE)
target_include_directories(block-deque INTERFACE ${PROJECT_SOURCE_DIR}/include)
target_sources(block-deque INTERFACE ${HEADER_PATH}/block_deque.h)

add_library(heap-timer INTERFACE)
target_include_directories(heap-timer INTERFACE ${PROJECT_SOURCE_DIR}/include)
target_sources(heap-timer INTERFACE ${HEADER_PATH}/heap_timer.h)

target_link_libraries(heap-timer
    INTERFACE
        log
        util
)

add_subdirectory(buffer)
add_subdirectory(thread_pool)
add_subdirectory(epoller)