cmake_minimum_required(VERSION 3.20)
project(FalcorComponents LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------
# Opcao: habilitar o Delta GigE Vision SDK (apenas para Kria)
# OFF por padrao — usa mock camera para builds no PC.
# ---------------------------------------------------------------
option(FALCOR_USE_DELTA_SDK "Habilitar Delta GigE Vision SDK (DMV)" OFF)

# Fontes base: tudo em src/ (exceto subpastas vendors/ e interfaces/
# que sao controladas separadamente)
file(GLOB COMP_CORE_SOURCES
    src/*.cpp
    src/interfaces/*.cpp
)

# Fonte do vendor escolhido
if(FALCOR_USE_DELTA_SDK)
    find_path(DELTA_INCLUDE_DIR
        NAMES dmvc/dmvc.h
        DOC "Caminho para os headers do Delta SDK"
    )
    find_library(DELTA_LIB
        NAMES dmvc-c
        DOC "Caminho para a libdmvc-c"
    )

    if(NOT DELTA_INCLUDE_DIR OR NOT DELTA_LIB)
        message(FATAL_ERROR
            "[COMPONENTS] FALCOR_USE_DELTA_SDK=ON mas o SDK nao foi encontrado.\n"
            "  Verifique que dmvc/dmvc.h e libdmvc-c estao no CMAKE_PREFIX_PATH."
        )
    endif()

    message(STATUS "[COMPONENTS] Delta SDK: ${DELTA_INCLUDE_DIR}")
    list(APPEND COMP_CORE_SOURCES src/vendors/ImplCameraComponent_DMV_SDK.cpp)
else()
    message(STATUS "[COMPONENTS] Delta SDK desativado — usando ImplCameraComponent_Mock")
    list(APPEND COMP_CORE_SOURCES src/vendors/ImplCameraComponent_Mock.cpp)
endif()

add_library(FalcorComponents STATIC ${COMP_CORE_SOURCES})

target_include_directories(FalcorComponents PUBLIC src)

target_link_libraries(FalcorComponents PUBLIC FalcorCore)

if(FALCOR_USE_DELTA_SDK)
    target_include_directories(FalcorComponents PUBLIC ${DELTA_INCLUDE_DIR})
    target_link_libraries(FalcorComponents PRIVATE ${DELTA_LIB})
    target_compile_definitions(FalcorComponents PUBLIC FALCOR_HAVE_DELTA_SDK)
endif()
