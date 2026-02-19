# pipeline/CMakeLists.txt
file(GLOB PIPE_SOURCES "src/*.cpp")

add_executable(Falcor ${PIPE_SOURCES})

# Ao linkar com FalcorCore (que agora tem ZMQ como PUBLIC), 
# o executável deve herdar tudo automaticamente.
target_link_libraries(Falcor PRIVATE 
    FalcorComponents
    FalcorCore
)