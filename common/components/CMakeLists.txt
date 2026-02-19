# common/components/CMakeLists.txt

# Captura os arquivos da pasta components
file(GLOB COMP_SOURCES "src/*.cpp")

# AQUI ESTÁ O ERRO: Mude de FalcorCore para FalcorComponents
add_library(FalcorComponents STATIC ${COMP_SOURCES})

# Configura os includes da pasta src dos componentes
target_include_directories(FalcorComponents PUBLIC "src")

# Linka com o Core (assim os componentes enxergam o que está no Core)
target_link_libraries(FalcorComponents PUBLIC FalcorCore)