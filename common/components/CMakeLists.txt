# common/components/CMakeLists.txt

# --- Busca Dinâmica do Delta SDK ---
# Procuramos por dmvc/dmvc.h para validar que o sysroot está correto
find_path(DELTA_INCLUDE_DIR 
    NAMES dmvc/dmvc.h
    DOC "Caminho para a raiz dos headers (onde reside a pasta dmvc)"
)

# Busca a biblioteca compilada
find_library(DELTA_LIB 
    NAMES dmvc-c
    DOC "Caminho para a biblioteca libdmvc-c"
)

# --- Definição da Biblioteca de Componentes ---
file(GLOB COMP_SOURCES "src/*.cpp")

add_library(FalcorComponents STATIC ${COMP_SOURCES})

# Configura os diretórios de include locais
target_include_directories(FalcorComponents PUBLIC "src")

# Se o SDK da Delta foi encontrado, adiciona os caminhos globais
if(DELTA_INCLUDE_DIR AND DELTA_LIB)
    message(STATUS "[COMPONENTS] Delta SDK localizado com sucesso em: ${DELTA_INCLUDE_DIR}")
    
    # Adicionamos DELTA_INCLUDE_DIR para que o código use #include "dmvc/dmvc.h"
    target_include_directories(FalcorComponents PUBLIC ${DELTA_INCLUDE_DIR})
else()
    message(WARNING "[COMPONENTS] Delta SDK não encontrado. Verifique se renomeou a pasta para 'dmvc' no sysroot.")
endif()

# --- Linkagem ---
# PUBLIC FalcorCore: Componentes herdam tudo do Core
# PRIVATE ${DELTA_LIB}: Linkagem da biblioteca da Delta
target_link_libraries(FalcorComponents 
    PUBLIC FalcorCore
    PRIVATE ${DELTA_LIB}
)