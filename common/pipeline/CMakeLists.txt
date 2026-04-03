cmake_minimum_required(VERSION 3.20)
project(FalcorPipeline LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB PIPE_SOURCES src/*.cpp)

add_library(FalcorPipeline STATIC ${PIPE_SOURCES})

target_include_directories(FalcorPipeline PUBLIC src)

target_link_libraries(FalcorPipeline PUBLIC
    FalcorComponents
    FalcorCore
)
