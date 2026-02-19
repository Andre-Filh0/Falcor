cmake_minimum_required (VERSION 3.20)

# 1. Habilite C e CXX aqui para o CMake carregar os dois compiladores (gcc e g++)
project ("Falcor" VERSION 0.1.0 LANGUAGES C CXX)

# --- Architecture Detection & Environment Setup ---

if(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
    add_compile_definitions(FALCOR_ENV_ARM64)
    message(STATUS "[FALCOR] Target: ARM64 (Kria). Interface types: uint32_t (HW Optimized).")

elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "AMD64")
    add_compile_definitions(FALCOR_ENV_X64)
    message(STATUS "[FALCOR] Target: x86_64 (PC). Interface types: size_t (Native Speed).")

else()
    message(STATUS "[FALCOR] Target: Generic/Unknown. Defaulting to size_t.")
endif()

# Garante que os binários saiam na pasta correta
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/out/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/out/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/out/build/lib")

# Configurações do C++
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 2. IMPORTANTE: A ordem dos subdirectories. 
# Como FalcorComponents depende de FalcorCore, e o Pipeline depende de ambos,
# a ordem lógica recomendada é:
add_subdirectory(common/core)       # Primeiro a base (Bluetooth/Rede)
add_subdirectory(common/components) # Depois os componentes que usam a base
add_subdirectory(common/pipeline)          # Por fim o executável que junta tudo