#pragma once

#include "Logger.hpp"
#include "System.hpp"
#include "../interfaces/ICameraComponent.hpp"
#include "dmvc/dmvc.h"

#include <atomic>
#include <vector>

#define DEVICE_LIST_UPDATE_TIMEOUT 1000
#define BUFFER_COUNT 16
#define IMAGE_WAIT_TIME 2000

class ImplCameraComponent_DMV_SDK final : public ICameraComponent
{
public:
    explicit ImplCameraComponent_DMV_SDK(const CameraConfig& config);
    ~ImplCameraComponent_DMV_SDK() override;

    CameraStatus connect(const CameraConfig& config) override;
    void disconnect() override;
    bool isConnected() const override;

    CameraStatus startStream() override;
    void stopStream() override;
    bool isStreaming() const override;

    CameraStatus getRawFrame(RawFrame& outFrame) override;

    uint32_t getWidth() const override;
    uint32_t getHeight() const override;
    PixelFormat getPixelFormat() const override;
    const char* getCameraName() const override;

private:
    void resetHandles() noexcept;

private:
    std::atomic<bool> m_connected{false};
    std::atomic<bool> m_streaming{false};

    CameraConfig m_config;
    uint32_t m_frameCounter{0};

    DcSystem     m_system{nullptr};
    DcDevice     m_device{nullptr};
    DcDataStream m_stream{nullptr};
    DcNodeList   m_nodelist{nullptr};

    std::vector<DcBuffer> m_buffers;
};