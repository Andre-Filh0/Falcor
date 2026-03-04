cmake_minimum_required(VERSION 3.20)
project("Falcor" VERSION 0.1.0 LANGUAGES C CXX)

# --- Configurações de Saída ---
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/out/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/out/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/out/build/lib")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- 1. Adicionar as Subpastas (Bibliotecas) ---
# Certifique-se que dentro de cada uma delas exista um add_library(...)
add_subdirectory(common/core)       
add_subdirectory(common/components) 
add_subdirectory(common/pipeline)   

# --- 2. Criar o Executável Principal no Root ---
# Apontando para o seu src/main.cpp na raiz do projeto
add_executable(Falcor "src/main.cpp")

# --- 3. Linkar tudo no Executável ---
target_link_libraries(Falcor PRIVATE 
    FalcorPipeline    # A biblioteca que o pipeline gerar
    FalcorComponents
    FalcorCore
)