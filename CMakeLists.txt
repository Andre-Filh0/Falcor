cmake_minimum_required(VERSION 3.20)
project(Falcor VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

add_subdirectory(common/core)
add_subdirectory(common/components)
add_subdirectory(common/pipeline)

add_executable(Falcor src/main.cpp)

target_link_libraries(Falcor PRIVATE
    FalcorPipeline
    FalcorComponents
    FalcorCore
)