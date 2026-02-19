set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

# Compiladores para Cross-Compilation
set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)

# Onde o sistema de arquivos ARM64 está montado/instalado
set(ARM_SYSROOT "/usr/lib/aarch64-linux-gnu")

# Configurações de busca: Força o CMake a olhar apenas para o Sysroot alvo
set(CMAKE_FIND_ROOT_PATH ${ARM_SYSROOT} /usr/aarch64-linux-gnu)

set(CMAKE_LIBRARY_PATH ${ARM_SYSROOT})
set(CMAKE_STAGING_PREFIX /usr/aarch64-linux-gnu)

# Modos de busca: NEVER para programas (usa os do PC), ONLY para libs/includes (usa os ARM64)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Garante que o linker encontre dependências indiretas (bibliotecas que outras bibliotecas usam)
set(CMAKE_EXE_LINKER_FLAGS "-Wl,-rpath-link,${ARM_SYSROOT}" CACHE STRING "" FORCE)