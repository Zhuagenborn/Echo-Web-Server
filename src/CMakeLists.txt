add_executable(${CMAKE_PROJECT_NAME})

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        main.cpp
)

add_subdirectory(util)
add_subdirectory(test_util)
add_subdirectory(containers)
add_subdirectory(io)
add_subdirectory(config)
add_subdirectory(log)