#pragma once
#include <cstdint>
#include <vector>
#include <memory> // Para usar smart pointers

namespace Falcor {

template <typename T, typename D>
struct RawFrame {
    std::vector<T> data;
    D width;
    D height;
    D stride;
    uint64_t timestamp;
    uint32_t frameId;

    bool empty() const { return data.empty(); }
};

class ICameraComponent {
protected:
    // Definição dos tipos ideais por arquitetura
    #if defined(FALCOR_ENV_ARM64)
        using DimType = uint32_t;  // Melhor para barramento AXI/FPGA
        using MemType = uint8_t;   // Imagem bruta padrão
    #else
        using DimType = size_t;    // Melhor para performance de CPU x86
        using MemType = uint8_t;
    #endif

public:
    virtual ~ICameraComponent() = default;

    virtual bool open() = 0;
    virtual void close() = 0;
    virtual bool isOpened() const = 0;

    virtual RawFrame<MemType, DimType> getRawFrame() = 0;

    virtual DimType getWidth() const = 0;
    virtual DimType getHeight() const = 0;
    
    virtual const char* getCameraName() const = 0;
};

}