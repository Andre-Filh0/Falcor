#pragma once

#include <cstdint>
#include <vector>
#include <memory>
#include <string>

// ============================================================
// Pixel Format
// ============================================================

enum class PixelFormat
{
    Unknown = 0,
    Mono8,
    Mono16,
    RGB8,
    BGR8,
    RGBA8,
    BayerRG8,
    BayerRG16,
    YUV422
};

// ============================================================
// Camera Status
// ============================================================

enum class CameraStatus
{
    Ok = 0,
    NotConnected,
    AlreadyConnected,
    AlreadyStreaming,
    NotStreaming,
    StreamNotStarted,
    Timeout,
    DeviceLost,
    InvalidConfiguration,
    InternalError
};

// ============================================================
// Frame Metadata
// ============================================================

struct FrameMetadata
{
    uint64_t timestamp = 0;
    uint32_t frameId   = 0;
};

// ============================================================
// Raw Frame
// ============================================================

struct RawFrame
{
    using DimType = uint32_t;
    using MemType = uint8_t;

    std::vector<MemType> data;

    DimType width  = 0;
    DimType height = 0;
    DimType stride = 0;

    PixelFormat format = PixelFormat::Unknown;
    FrameMetadata metadata;

    bool empty() const noexcept { return data.empty(); }
    size_t sizeBytes() const noexcept { return data.size(); }

    MemType* ptr() noexcept { return data.data(); }
    const MemType* ptr() const noexcept { return data.data(); }
};

// ============================================================
// Camera Configuration
// ============================================================

struct CameraConfig
{
    uint32_t width;
    uint32_t height;
    PixelFormat format;
    uint32_t fps;

    CameraConfig(
        uint32_t w,
        uint32_t h,
        PixelFormat fmt,
        uint32_t framesPerSecond
    )
        : width(w),
          height(h),
          format(fmt),
          fps(framesPerSecond)
    {}
};

// ============================================================
// ICameraComponent
// ============================================================

class ICameraComponent
{
public:
    virtual ~ICameraComponent() = default;

    // --------------------------------------------------------
    // Device Lifecycle
    // --------------------------------------------------------

    virtual CameraStatus connect(const CameraConfig& config) = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() const = 0;

    // --------------------------------------------------------
    // Streaming Lifecycle
    // --------------------------------------------------------

    virtual CameraStatus startStream() = 0;
    virtual void stopStream() = 0;
    virtual bool isStreaming() const = 0;

    // --------------------------------------------------------
    // Frame Acquisition
    // --------------------------------------------------------

    virtual CameraStatus getRawFrame(RawFrame& outFrame) = 0;

    // --------------------------------------------------------
    // Info
    // --------------------------------------------------------

    virtual uint32_t getWidth() const = 0;
    virtual uint32_t getHeight() const = 0;
    virtual PixelFormat getPixelFormat() const = 0;

    virtual const char* getCameraName() const = 0;
};