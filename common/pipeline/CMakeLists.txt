# pipeline/CMakeLists.txt
file(GLOB PIPE_SOURCES "src/*.cpp")

# Transformamos em library em vez de executable
add_library(FalcorPipeline STATIC ${PIPE_SOURCES})

target_link_libraries(FalcorPipeline PUBLIC 
    FalcorComponents
    FalcorCore
)