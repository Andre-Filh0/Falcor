cmake_minimum_required(VERSION 3.20)
project(FalcorCore LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

# cppzmq engloba libzmq + zmq.hpp e exporta o target 'cppzmq'
# (ZeroMQ::ZeroMQ nao existe no config do nixpkgs/zeromq 4.x)
find_package(cppzmq CONFIG REQUIRED)

find_package(quill CONFIG REQUIRED)

# FalcorCore e header-only — INTERFACE propaga includes e deps sem compilar nada.
add_library(FalcorCore INTERFACE)

target_include_directories(FalcorCore INTERFACE src)

target_link_libraries(FalcorCore INTERFACE
    Threads::Threads
    cppzmq
    quill::quill
)
