#include "ImplCameraComponent_DMV_SDK.hpp"

// Certifique-se de incluir o cabeçalho do seu executor CLI
// #include "SuaPasta/SystemCLI.hpp" 

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
CameraStatus ImplCameraComponent_DMV_SDK::connect(const CameraConfig& config)
{
    if (m_connected.load()) return CameraStatus::AlreadyConnected;

    m_config = config;
    const char* targetIP = m_config.protocol.GigE.camera_ip_str;

    Falcor::Logger.log_info("[DMV_SDK] Iniciando conexao para o IP alvo: {}", targetIP);

    resetHandles();
    DcError err = DcSystemCreate(&m_system);
    if (err != DC_ERROR_SUCCESS) return CameraStatus::InternalError;

    // Atualiza interfaces de rede
    DcSystemUpdateInterfaceList(m_system, nullptr, DC_INFINITE);
    DcSystemGetInterfaceCount(m_system, &m_interface_count);

    // Loop pelas interfaces físicas do Kria
    for (uint32_t i = 0; i < m_interface_count; ++i)
    {
        err = DcSystemGetInterface(m_system, i, &m_interface);
        if (err != DC_ERROR_SUCCESS) continue;

        if (DcInterfaceOpen(m_interface) != DC_ERROR_SUCCESS) continue;

        // Procura dispositivos nesta interface específica
        DcInterfaceUpdateDeviceList(m_interface, nullptr, DEVICE_LIST_UPDATE_TIMEOUT);
        DcInterfaceGetDeviceCount(m_interface, &m_device_count);

        for (uint32_t j = 0; j < m_device_count; ++j)
        {
            err = DcInterfaceGetDevice(m_interface, j, &m_device);
            if (err != DC_ERROR_SUCCESS) continue;

            // --- Lógica de Filtro por IP ---
            char discoveredIP[256] = { 0 };
            size_t size = sizeof(discoveredIP);
            
            // Acessando o IP do dispositivo antes de abrir a conexão de controle
            DcNodeList nodelist;
            if (DcDeviceGetNodeList(m_device, &nodelist) == DC_ERROR_SUCCESS) {
                 DcNodeListGetValue(nodelist, "GevDeviceIPAddress", discoveredIP, &size);
            }

            // Se o IP descoberto NÃO for o IP que passamos no main, ignoramos este device
            if (std::string(discoveredIP) != std::string(targetIP)) {
                m_device = nullptr; 
                continue; 
            }

            // Se chegamos aqui, o IP bate! Agora testamos reachability
            std::string pingCmd = "ping -c 1 -W 1 " + std::string(discoveredIP) + " > /dev/null 2>&1";
            Falcor::System::CLI::executeCLICommand(pingCmd.c_str());

            // Tenta abrir o controle da câmera
            err = DcDeviceOpen(m_device, DC_DEVICE_ACCESS_TYPE_CONTROL);
            if (err == DC_ERROR_SUCCESS)
            {
                m_connected.store(true);
                Falcor::Logger.log_info("[DMV_SDK] Camera {} encontrada e conectada!", discoveredIP);
                return CameraStatus::Ok;
            }
        }
        
        DcInterfaceClose(m_interface);
        m_interface = nullptr;
    }

    Falcor::Logger.log_error("[DMV_SDK] Erro: Camera com IP {} nao foi encontrada na rede.", targetIP);
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

    Falcor::Logger.log_info("[DMV_SDK] Device disconnected successfully.");
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