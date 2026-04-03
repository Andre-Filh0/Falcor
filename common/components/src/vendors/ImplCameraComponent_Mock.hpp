#pragma once

#include "interfaces/ICameraComponent.hpp"
#include <atomic>
#include <cstdint>

/**
 * @brief Implementacao mock da camera para desenvolvimento no PC.
 *
 * Gera frames sinteticos (padrao xadrez animado) sem qualquer hardware.
 * Ativada automaticamente quando FALCOR_USE_DELTA_SDK=OFF.
 */
class ImplCameraComponent_Mock : public ICameraComponent
{
public:
    explicit ImplCameraComponent_Mock(const CameraConfig& config);
    ~ImplCameraComponent_Mock() override = default;

    CameraStatus connect(const CameraConfig& config) override;
    void         disconnect() override;
    bool         isConnected() const override;

    CameraStatus startStream() override;
    void         stopStream() override;
    bool         isStreaming() const override;

    CameraStatus getRawFrame(RawFrame& outFrame) override;

    uint32_t     getWidth()       const override;
    uint32_t     getHeight()      const override;
    PixelFormat  getPixelFormat() const override;
    const char*  getCameraName()  const override;

private:
    CameraConfig          m_config;
    std::atomic<bool>     m_connected{false};
    std::atomic<bool>     m_streaming{false};
    uint32_t              m_frameCounter{0};
};
