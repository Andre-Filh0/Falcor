#include "ImplCameraComponent_DMV_SDK.hpp"
#include <cstring>

// ============================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================

ImplCameraComponent_DMV_SDK::ImplCameraComponent_DMV_SDK(const CameraConfig& config)
    : m_config(config)
{
    resetHandles();
}

ImplCameraComponent_DMV_SDK::~ImplCameraComponent_DMV_SDK()
{
    disconnect();
}

// ============================================================
// CONNECT
// ============================================================

CameraStatus ImplCameraComponent_DMV_SDK::connect(const CameraConfig& config)
{
    if (m_connected.load())
        return CameraStatus::AlreadyConnected;

    m_config = config;

    if (DcSystemCreate(&m_system) != DC_ERROR_SUCCESS)
        return CameraStatus::InternalError;

    // BUG FIX #4: Pass the GigE IP address so the SDK connects to the correct camera.
    // Previously nullptr was passed, causing the SDK to pick an arbitrary device.
    const char* camera_ip = m_config.protocol.GigE.camera_ip.c_str();
    if (DcSystemGetDevice(m_system, camera_ip, &m_device) != DC_ERROR_SUCCESS)
        return CameraStatus::NotConnected;

    if (DcDeviceOpen(m_device, DC_DEVICE_ACCESS_TYPE_CONTROL) != DC_ERROR_SUCCESS)
        return CameraStatus::InternalError;

    if (DcDeviceGetRemoteNodeList(m_device, &m_nodelist) != DC_ERROR_SUCCESS)
        return CameraStatus::InternalError;

    if (DcDeviceGetDataStream(m_device, 0, &m_stream) != DC_ERROR_SUCCESS)
        return CameraStatus::InternalError;

    m_connected.store(true);

    Falcor::Logger.log_info("[DMV] Camera connected successfully (IP: {}).", camera_ip);
    return CameraStatus::Ok;
}

// ============================================================
// START STREAM
// ============================================================

CameraStatus ImplCameraComponent_DMV_SDK::startStream()
{
    if (!m_connected.load())
        return CameraStatus::NotConnected;

    if (m_streaming.load())
        return CameraStatus::AlreadyStreaming;

    if (DcNodeListSetValue(m_nodelist, "AcquisitionMode", "Continuous") != DC_ERROR_SUCCESS)
        return CameraStatus::InternalError;

    if (DcNodeListSetSelectedValue(m_nodelist, "TriggerSelector", "", "TriggerMode", "Off") != DC_ERROR_SUCCESS)
        return CameraStatus::InternalError;

    m_buffers.clear();

    for (uint32_t i = 0; i < BUFFER_COUNT; i++)
    {
        DcBuffer buffer;

        if (DcDataStreamAllocAndAnnounceBuffer(m_stream, nullptr, &buffer) != DC_ERROR_SUCCESS)
            return CameraStatus::InternalError;

        if (DcDataStreamQueueBuffer(m_stream, buffer) != DC_ERROR_SUCCESS)
            return CameraStatus::InternalError;

        m_buffers.push_back(buffer);
    }

    if (DcDataStreamStartAcquisition(m_stream) != DC_ERROR_SUCCESS)
        return CameraStatus::InternalError;

    m_streaming.store(true);

    Falcor::Logger.log_info("[DMV] Streaming iniciado.");
    return CameraStatus::Ok;
}

// ============================================================
// GET FRAME (VERSÃO FINAL COMPATÍVEL)
// ============================================================
CameraStatus ImplCameraComponent_DMV_SDK::getRawFrame(RawFrame& outFrame)
{
    if (!m_streaming.load())
        return CameraStatus::NotStreaming;

    DcBuffer buffer = nullptr;

    DcError error = DcDataStreamGetFilledBuffer(m_stream, IMAGE_WAIT_TIME, &buffer);

    if (error == DC_ERROR_TIMEOUT)
        return CameraStatus::Timeout;

    if (error != DC_ERROR_SUCCESS)
        return CameraStatus::InternalError;

    bool is_complete = false;

    if (DcBufferIsComplete(buffer, &is_complete) != DC_ERROR_SUCCESS || !is_complete)
    {
        DcDataStreamQueueBuffer(m_stream, buffer);
        return CameraStatus::InternalError;
    }

    DcImage image = nullptr;

    if (DcBufferGetImage(buffer, &image) != DC_ERROR_SUCCESS || !image)
    {
        DcDataStreamQueueBuffer(m_stream, buffer);
        return CameraStatus::InternalError;
    }

    void* data_ptr = nullptr;

    if (DcImageGetDataPtr(image, &data_ptr) != DC_ERROR_SUCCESS || !data_ptr)
    {
        DcDataStreamQueueBuffer(m_stream, buffer);
        return CameraStatus::InternalError;
    }

    size_t width  = 0;
    size_t height = 0;
    size_t dataSize = 0;

    if (DcImageGetWidth(image, &width) != DC_ERROR_SUCCESS)
    {
        DcDataStreamQueueBuffer(m_stream, buffer);
        return CameraStatus::InternalError;
    }

    if (DcImageGetHeight(image, &height) != DC_ERROR_SUCCESS)
    {
        DcDataStreamQueueBuffer(m_stream, buffer);
        return CameraStatus::InternalError;
    }

    // Função correta para o SDK instalado
    if (DcImageGetDataSize(image, &dataSize) != DC_ERROR_SUCCESS)
    {
        DcDataStreamQueueBuffer(m_stream, buffer);
        return CameraStatus::InternalError;
    }

    if (dataSize == 0)
    {
        DcDataStreamQueueBuffer(m_stream, buffer);
        return CameraStatus::InternalError;
    }

    // Copiar exatamente o tamanho retornado pelo SDK
    outFrame.data.resize(dataSize);
    std::memcpy(outFrame.data.data(), data_ptr, dataSize);

    outFrame.width  = static_cast<uint32_t>(width);
    outFrame.height = static_cast<uint32_t>(height);
    outFrame.format = m_config.format;

    m_frameCounter++;

    // Recoloca o buffer na fila
    DcDataStreamQueueBuffer(m_stream, buffer);

    return CameraStatus::Ok;
}

// ============================================================
// STOP STREAM
// ============================================================

void ImplCameraComponent_DMV_SDK::stopStream()
{
    if (!m_streaming.load())
        return;

    DcDataStreamStopAcquisition(m_stream, false);

    // BUG FIX #5: Revoke each buffer before clearing the vector.
    // Calling m_buffers.clear() without revoking leaks SDK-internal memory
    // because the SDK still holds references to the announced buffers.
    for (DcBuffer buf : m_buffers) {
        DcDataStreamRevokeBuffer(m_stream, buf, nullptr, nullptr);
    }
    m_buffers.clear();

    m_streaming.store(false);

    Falcor::Logger.log_info("[DMV] Streaming stopped.");
}

// ============================================================
// DISCONNECT
// ============================================================

void ImplCameraComponent_DMV_SDK::disconnect()
{
    stopStream();

    if (m_device)
    {
        DcDeviceClose(m_device);
        m_device = nullptr;
    }

    if (m_system)
    {
        DcSystemDestroy(m_system);
        m_system = nullptr;
    }

    resetHandles();

    m_connected.store(false);
}

// ============================================================
// GETTERS
// ============================================================

bool ImplCameraComponent_DMV_SDK::isConnected() const
{
    return m_connected.load();
}

bool ImplCameraComponent_DMV_SDK::isStreaming() const
{
    return m_streaming.load();
}

uint32_t ImplCameraComponent_DMV_SDK::getWidth() const
{
    return m_config.width;
}

uint32_t ImplCameraComponent_DMV_SDK::getHeight() const
{
    return m_config.height;
}

PixelFormat ImplCameraComponent_DMV_SDK::getPixelFormat() const
{
    return m_config.format;
}

const char* ImplCameraComponent_DMV_SDK::getCameraName() const
{
    return "DMV GigE Vision Camera";
}

// ============================================================
// RESET HANDLES
// ============================================================

void ImplCameraComponent_DMV_SDK::resetHandles() noexcept
{
    m_system   = nullptr;
    m_device   = nullptr;
    m_stream   = nullptr;
    m_nodelist = nullptr;
}