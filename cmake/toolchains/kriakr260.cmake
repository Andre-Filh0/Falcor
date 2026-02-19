set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)

# Onde o apt jogou tudo
set(ARM_SYSROOT "/usr/lib/aarch64-linux-gnu")

# Força o CMake a olhar o diretório de pacotes ARM64
set(CMAKE_LIBRARY_PATH ${ARM_SYSROOT})
set(CMAKE_STAGING_PREFIX /usr/aarch64-linux-gnu)

# DIRECIONAMENTO EXPLÍCITO PARA O QT6 ARM64
set(Qt6_DIR "${ARM_SYSROOT}/cmake/Qt6")
set(Qt6Core_DIR "${ARM_SYSROOT}/cmake/Qt6Core")
set(Qt6Bluetooth_DIR "${ARM_SYSROOT}/cmake/Qt6Bluetooth")

# Adiciona ao prefixo de busca
list(APPEND CMAKE_PREFIX_PATH "${ARM_SYSROOT}/cmake")

set(CMAKE_FIND_ROOT_PATH ${ARM_SYSROOT})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

set(CMAKE_EXE_LINKER_FLAGS "-Wl,-rpath-link,${ARM_SYSROOT}" CACHE STRING "" FORCE)