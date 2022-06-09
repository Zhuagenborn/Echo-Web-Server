add_library(thread-pool)

set(HEADER_PATH ${PROJECT_SOURCE_DIR}/include/containers)

target_include_directories(thread-pool PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_include_directories(thread-pool PRIVATE ${HEADER_PATH})

target_sources(thread-pool
    PUBLIC
        ${HEADER_PATH}/thread_pool.h
    PRIVATE
        thread_pool.cpp
)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(thread-pool PRIVATE Threads::Threads)

target_link_libraries(thread-pool
    PUBLIC
        log
    PRIVATE
        util
)