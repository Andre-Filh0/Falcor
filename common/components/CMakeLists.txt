# common/components/CMakeLists.txt

# --- Busca Dinâmica do Delta SDK ---
# O find_path procurará por 'delta/dmvc.h' dentro do ARM_SYSROOT/include
find_path(DELTA_INCLUDE_DIR 
    NAMES delta/dmvc.h
    DOC "Caminho para os headers da Delta"
)

# O find_library procurará por 'libdmvc-c.so' no ARM_SYSROOT/lib
find_library(DELTA_LIB 
    NAMES dmvc-c
    DOC "Caminho para a biblioteca libdmvc-c"
)

# --- Definição da Biblioteca de Componentes ---
file(GLOB COMP_SOURCES "src/*.cpp")

add_library(FalcorComponents STATIC ${COMP_SOURCES})

# Configura os diretórios de include
target_include_directories(FalcorComponents PUBLIC "src")

# Se o SDK da Delta foi encontrado, adiciona os caminhos
if(DELTA_INCLUDE_DIR AND DELTA_LIB)
    message(STATUS "[COMPONENTS] Delta SDK localizado!")
    target_include_directories(FalcorComponents PUBLIC ${DELTA_INCLUDE_DIR})
else()
    message(WARNING "[COMPONENTS] Delta SDK não encontrado. Verifique se os arquivos .h e .so estão no Sysroot.")
endif()

# --- Linkagem ---
# PUBLIC FalcorCore: Componentes herdam tudo do Core
# PRIVATE ${DELTA_LIB}: Componentes usam a Delta internamente
target_link_libraries(FalcorComponents 
    PUBLIC FalcorCore
    PRIVATE ${DELTA_LIB}
)