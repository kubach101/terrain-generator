cmake_minimum_required(VERSION 3.10)

project(terrain C)

set(CMAKE_PREFIX_PATH "C:/SDL3")

find_package(SDL3 REQUIRED CONFIG)

add_executable(terrain
    src/main.c
)

target_link_libraries(terrain PRIVATE
    SDL3::SDL3
)
set_target_properties(terrain PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/bin"
)