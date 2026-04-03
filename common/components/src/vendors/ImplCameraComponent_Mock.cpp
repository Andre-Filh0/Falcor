#include "ImplCameraComponent_Mock.hpp"
#include "Logger.hpp"

ImplCameraComponent_Mock::ImplCameraComponent_Mock(const CameraConfig& config)
    : m_config(config)
{}

CameraStatus ImplCameraComponent_Mock::connect(const CameraConfig& config)
{
    if (m_connected.load())
        return CameraStatus::AlreadyConnected;

    m_config = config;
    m_connected.store(true);

    Falcor::Logger.log_info("[MockCamera] Conectada ({}x{}, mock).",
        config.width, config.height);
    return CameraStatus::Ok;
}

void ImplCameraComponent_Mock::disconnect()
{
    stopStream();
    m_connected.store(false);
    Falcor::Logger.log_info("[MockCamera] Desconectada.");
}

bool ImplCameraComponent_Mock::isConnected() const
{
    return m_connected.load();
}

CameraStatus ImplCameraComponent_Mock::startStream()
{
    if (!m_connected.load())  return CameraStatus::NotConnected;
    if (m_streaming.load())   return CameraStatus::AlreadyStreaming;

    m_streaming.store(true);
    Falcor::Logger.log_info("[MockCamera] Stream iniciado.");
    return CameraStatus::Ok;
}

void ImplCameraComponent_Mock::stopStream()
{
    if (!m_streaming.load()) return;
    m_streaming.store(false);
    Falcor::Logger.log_info("[MockCamera] Stream parado.");
}

bool ImplCameraComponent_Mock::isStreaming() const
{
    return m_streaming.load();
}

CameraStatus ImplCameraComponent_Mock::getRawFrame(RawFrame& outFrame)
{
    if (!m_streaming.load())
        return CameraStatus::NotStreaming;

    const uint32_t w             = m_config.width;
    const uint32_t h             = m_config.height;
    constexpr uint32_t kBPP      = 3; // RGB8: 3 bytes por pixel
    const size_t       dataSize  = static_cast<size_t>(w) * h * kBPP;

    outFrame.data.resize(dataSize);
    outFrame.width  = w;
    outFrame.height = h;
    outFrame.stride = w * kBPP;
    outFrame.format = PixelFormat::RGB8;

    // Padrao xadrez 64x64 que avanca a cada frame (confirma que frames mudam)
    for (uint32_t y = 0; y < h; ++y) {
        for (uint32_t x = 0; x < w; ++x) {
            const bool on  = ((x / 64u) + (y / 64u) + m_frameCounter) % 2u == 0u;
            const size_t i = (static_cast<size_t>(y) * w + x) * kBPP;
            outFrame.data[i + 0] = on ? 200u : 40u;   // R
            outFrame.data[i + 1] = on ? 80u  : 160u;  // G
            outFrame.data[i + 2] = on ? 40u  : 200u;  // B
        }
    }

    outFrame.metadata.frameId   = m_frameCounter;
    outFrame.metadata.timestamp = static_cast<uint64_t>(m_frameCounter) * 16667ULL; // ~60 fps em us

    ++m_frameCounter;
    return CameraStatus::Ok;
}

uint32_t    ImplCameraComponent_Mock::getWidth()       const { return m_config.width; }
uint32_t    ImplCameraComponent_Mock::getHeight()      const { return m_config.height; }
PixelFormat ImplCameraComponent_Mock::getPixelFormat() const { return m_config.format; }
const char* ImplCameraComponent_Mock::getCameraName()  const { return "Mock Camera (PC Development)"; }
