add_executable(${CMAKE_PROJECT_NAME})

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        main.cpp
)

set(HEADER_PATH ${PROJECT_SOURCE_DIR}/include)

add_library(web-server INTERFACE)
target_include_directories(web-server INTERFACE ${HEADER_PATH})
target_sources(web-server INTERFACE ${HEADER_PATH}/web_server.h)

target_link_libraries(web-server
    INTERFACE
        epoller
        heap-timer
        thread-pool
        http
        ip
        log
        util
)

add_subdirectory(util)
add_subdirectory(test_util)
add_subdirectory(containers)
add_subdirectory(io)
add_subdirectory(config)
add_subdirectory(log)
add_subdirectory(ip)
add_subdirectory(http)