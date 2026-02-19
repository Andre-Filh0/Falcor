cmake_minimum_required (VERSION 3.20)

# 1. Habilite C e CXX aqui para o CMake carregar os dois compiladores (gcc e g++)
project ("Falcor" VERSION 0.1.0 LANGUAGES C CXX)

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