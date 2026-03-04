#pragma once

#include "Logger.hpp"
#include "System.hpp"
#include "../interfaces/ICameraComponent.hpp"
#include "dmvc/dmvc.h"

#include <atomic>
#include <cstddef>

#define DEVICE_LIST_UPDATE_TIMEOUT 1000 /* milliseconds */

// ============================================================
// ImplCameraComponent_DMV_SDK
// ============================================================

class ImplCameraComponent_DMV_SDK final : public ICameraComponent
{
public:
    explicit ImplCameraComponent_DMV_SDK(const CameraConfig& config);
    ~ImplCameraComponent_DMV_SDK() override;

    // --------------------------------------------------------
    // Device Lifecycle
    // --------------------------------------------------------

    CameraStatus connect(const CameraConfig& config) override;
    void disconnect() override;
    bool isConnected() const override;

    // --------------------------------------------------------
    // Streaming Lifecycle
    // --------------------------------------------------------

    CameraStatus startStream() override;
    void stopStream() override;
    bool isStreaming() const override;

    // --------------------------------------------------------
    // Frame Acquisition
    // --------------------------------------------------------

    CameraStatus getRawFrame(RawFrame& outFrame) override;

    // --------------------------------------------------------
    // Info
    // --------------------------------------------------------

    uint32_t getWidth() const override;
    uint32_t getHeight() const override;
    PixelFormat getPixelFormat() const override;
    const char* getCameraName() const override;

private:

    void resetHandles() noexcept;
    bool openFirstAvailableDevice();
    void cleanupSystem();

private:

    std::atomic<bool> m_connected{false};
    std::atomic<bool> m_streaming{false};

    CameraConfig m_config;
    uint32_t m_frameCounter{0};

    DcSystem    m_system{nullptr};
    DcInterface m_interface{nullptr};
    DcDevice    m_device{nullptr};

    size_t m_interface_count{0};
    size_t m_device_count{0};
};