rm -rf out/

# Configura (Passo 1)
cmake -S . -B out/build -G Ninja -DCMAKE_TOOLCHAIN_FILE=./cmake/toolchains/kriakr260.cmake -DCMAKE_BUILD_TYPE=Release

# Compila (Passo 2)
cmake --build out/build