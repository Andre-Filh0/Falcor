cmake_minimum_required(VERSION 3.20)
project(FalcorPipeline LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(FALCOR_USE_VITIS_AI "Habilitar inferencia Vitis AI DPU (apenas Kria KR260)" OFF)

# Fontes base da pipeline
set(PIPE_SOURCES
    src/DoubleBuffer.cpp
    src/Pipeline.cpp
    src/CPipelineComponent.cpp
    src/steps/VitisAIStep.cpp
)

add_library(FalcorPipeline STATIC ${PIPE_SOURCES})

target_include_directories(FalcorPipeline PUBLIC
    src
    src/steps
)

target_link_libraries(FalcorPipeline PUBLIC
    FalcorComponents
    FalcorCore
)

if(FALCOR_USE_VITIS_AI)
    find_path(VITIS_AI_INCLUDE_DIR
        NAMES vart/runner.hpp
        DOC "Caminho para os headers do Vitis AI Runtime (VART)"
    )
    find_library(VART_LIB       NAMES vart)
    find_library(XIR_LIB        NAMES xir)
    find_library(TARGET_FACTORY NAMES target_factory)

    if(NOT VITIS_AI_INCLUDE_DIR OR NOT VART_LIB OR NOT XIR_LIB)
        message(FATAL_ERROR
            "[PIPELINE] FALCOR_USE_VITIS_AI=ON mas VART/XIR nao foram encontrados.\n"
            "  Verifique que vart/runner.hpp, libvart e libxir estao no CMAKE_PREFIX_PATH."
        )
    endif()

    message(STATUS "[PIPELINE] Vitis AI encontrado: ${VITIS_AI_INCLUDE_DIR}")

    target_include_directories(FalcorPipeline PUBLIC ${VITIS_AI_INCLUDE_DIR})
    target_link_libraries(FalcorPipeline PRIVATE ${VART_LIB} ${XIR_LIB} ${TARGET_FACTORY})
    target_compile_definitions(FalcorPipeline PUBLIC FALCOR_HAVE_VITIS_AI)
else()
    message(STATUS "[PIPELINE] Vitis AI desativado — VitisAIStep em modo passthrough")
endif()
