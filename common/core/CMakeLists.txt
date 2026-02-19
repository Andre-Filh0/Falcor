cmake_minimum_required(VERSION 3.16)
project(FalcorCore LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON) 
# AJUSTE 1: Garante que o CMake encontre os arquivos gerados pelo MOC
set(CMAKE_INCLUDE_CURRENT_DIR ON) 

# --- Quill (Logging) via FetchContent ---
include(FetchContent)
FetchContent_Declare(
  quill
  GIT_REPOSITORY https://github.com/odygrd/quill.git
  GIT_TAG v3.7.0
)
FetchContent_MakeAvailable(quill)

# --- Dependências ---
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_check_modules(ZMQ REQUIRED libzmq)

# --- Configuração do Qt6 ---
set(QT_DIR_SEARCH "${ARM_DEPS}/usr/lib/aarch64-linux-gnu/cmake/Qt6")
find_package(Qt6 REQUIRED COMPONENTS Core Bluetooth PATHS ${QT_DIR_SEARCH} NO_DEFAULT_PATH)

# --- Definição da Biblioteca ---
add_library(FalcorCore STATIC
    src/Framework.hpp
)

# Inclusão de diretórios de headers
target_include_directories(FalcorCore PUBLIC 
    src
    ${ZMQ_INCLUDE_DIRS}
)

# Linkagem das dependências
target_link_libraries(FalcorCore PUBLIC 
    Qt6::Core 
    Qt6::Bluetooth
    ${ZMQ_LIBRARIES}
    quill::quill
    Threads::Threads
)