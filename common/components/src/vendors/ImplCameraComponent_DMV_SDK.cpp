#include "ImplCameraComponent_DMV_SDK.hpp"

// ============================================================
// Constructor / Destructor
// ============================================================

ImplCameraComponent_DMV_SDK::ImplCameraComponent_DMV_SDK(const CameraConfig& config)
    : m_config(config)
{
}

ImplCameraComponent_DMV_SDK::~ImplCameraComponent_DMV_SDK()
{
    disconnect();
}

// ============================================================
// CONNECT
// ============================================================
// ============================================================
// CONNECT
// ============================================================

CameraStatus ImplCameraComponent_DMV_SDK::connect(const CameraConfig& config)
{
    if (m_connected.load())
        return CameraStatus::AlreadyConnected;

    m_config = config;
    resetHandles();

    DcError err;

    // 1️⃣ Create system
    err = DcSystemCreate(&m_system);
    if (err != DC_ERROR_SUCCESS)
    {
        Falcor::Logger.log_error("DcSystemCreate failed.");
        return CameraStatus::InternalError;
    }

    // 2️⃣ Update interface list
    err = DcSystemUpdateInterfaceList(m_system, nullptr, DC_INFINITE);
    if (err != DC_ERROR_SUCCESS)
    {
        cleanupSystem();
        return CameraStatus::InternalError;
    }

    err = DcSystemGetInterfaceCount(m_system, &m_interface_count);
    if (err != DC_ERROR_SUCCESS || m_interface_count == 0)
    {
        Falcor::Logger.log_error("Nenhuma interface de rede encontrada pelo SDK.");
        cleanupSystem();
        return CameraStatus::NotConnected;
    }

    // 3️⃣ Iterate interfaces
    for (uint32_t i = 0; i < m_interface_count; ++i)
    {
        err = DcSystemGetInterface(m_system, i, &m_interface);
        if (err != DC_ERROR_SUCCESS)
            continue;

        err = DcInterfaceOpen(m_interface);
        if (err != DC_ERROR_SUCCESS)
        {
            m_interface = nullptr;
            continue;
        }

        DcInterfaceUpdateDeviceList(m_interface, nullptr, DEVICE_LIST_UPDATE_TIMEOUT);
        DcInterfaceGetDeviceCount(m_interface, &m_device_count);

        if (m_device_count == 0)
        {
            DcInterfaceClose(m_interface);
            m_interface = nullptr;
            continue;
        }

        // Try first device
        err = DcInterfaceGetDevice(m_interface, 0, &m_device);
        if (err != DC_ERROR_SUCCESS)
        {
            Falcor::Logger.log_error("Falha ao pegar o device 0 na interface {}. Erro: {}", i, err);
            DcInterfaceClose(m_interface);
            m_interface = nullptr;
            continue;
        }

        // --- VERIFICAÇÃO DE IP ---
        DcNodeList nodelist;
        DcNode node;
        DcIntegerNode device_selector;
        char camIP[256] = { 0 };
        char macAddr[256] = { 0 };
        size_t size = sizeof(camIP);

        // Pega as informações da câmera através dos Nodes
        if (DcInterfaceGetNodeList(m_interface, &nodelist) == DC_ERROR_SUCCESS) {
            if (DcNodeListGetNode(nodelist, "DeviceSelector", &node) == DC_ERROR_SUCCESS) {
                device_selector = DcNodeCastToIntegerNode(node);
                DcIntegerNodeSetValue(device_selector, 0); // Seleciona o device 0
                
                size = sizeof(camIP);
                DcNodeListGetValue(nodelist, "GevDeviceIPAddress", camIP, &size);
                
                size = sizeof(macAddr);
                DcNodeListGetValue(nodelist, "GevDeviceMACAddress", macAddr, &size);

                Falcor::Logger.log_info("================================");
                Falcor::Logger.log_info("Câmera encontrada na rede!");
                Falcor::Logger.log_info("IP da Câmera: {}", camIP);
                Falcor::Logger.log_info("MAC da Câmera: {}", macAddr);
                Falcor::Logger.log_info("================================");
            }
        }

        // Tenta abrir a câmera. É AQUI que falha se os IPs não baterem
        err = DcDeviceOpen(m_device, DC_DEVICE_ACCESS_TYPE_CONTROL);
        if (err != DC_ERROR_SUCCESS)
        {
            Falcor::Logger.log_error("Falha no DcDeviceOpen. Erro: {}", err);
            Falcor::Logger.log_error("VERIFIQUE: O IP do Ubuntu (Kria) está na mesma sub-rede do IP da Câmera ({})?", camIP);
            
            m_device = nullptr;
            DcInterfaceClose(m_interface);
            m_interface = nullptr;
            continue;
        }

        // 🎉 SUCCESS
        m_connected.store(true);
        Falcor::Logger.log_info("DMV camera connected successfully.");
        return CameraStatus::Ok;
    }

    // ❌ No device found
    Falcor::Logger.log_error("Fim do loop de interfaces. Nenhuma câmera pôde ser aberta.");
    cleanupSystem();
    return CameraStatus::NotConnected;
}

// ============================================================
// DISCONNECT
// ============================================================

void ImplCameraComponent_DMV_SDK::disconnect()
{
    if (!m_connected.load())
        return;

    stopStream();

    if (m_device)
    {
        DcDeviceClose(m_device);
        m_device = nullptr;
    }

    if (m_interface)
    {
        DcInterfaceClose(m_interface);
        m_interface = nullptr;
    }

    cleanupSystem();

    m_connected.store(false);

    Falcor::Logger.log_info("DMV camera disconnected.");
}

// ============================================================
// Helpers
// ============================================================

void ImplCameraComponent_DMV_SDK::cleanupSystem()
{
    if (m_system)
    {
        DcSystemDestroy(m_system);
        m_system = nullptr;
    }
}

void ImplCameraComponent_DMV_SDK::resetHandles() noexcept
{
    m_system = nullptr;
    m_interface = nullptr;
    m_device = nullptr;
}

CameraStatus ImplCameraComponent_DMV_SDK::startStream()
{
    return CameraStatus::Ok;
}

void ImplCameraComponent_DMV_SDK::stopStream()
{
}

bool ImplCameraComponent_DMV_SDK::isConnected() const
{
    return m_connected.load();
}

bool ImplCameraComponent_DMV_SDK::isStreaming() const
{
    return false;
}

CameraStatus ImplCameraComponent_DMV_SDK::getRawFrame(RawFrame&)
{
    return CameraStatus::NotStreaming;
}

// ============================================================
// INFO
// ============================================================

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
    return "DMV Camera";
}