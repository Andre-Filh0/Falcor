#include "ImplCameraComponent_DMV_SDK.hpp"
#include <cstring>
#include <cstdio>

// ─────────────────────────────────────────────────────────────────────────────
// Constructor / Destructor
// ─────────────────────────────────────────────────────────────────────────────

ImplCameraComponent_DMV_SDK::ImplCameraComponent_DMV_SDK(const CameraConfig& config)
    : m_config(config)
{
    resetHandles();
}

ImplCameraComponent_DMV_SDK::~ImplCameraComponent_DMV_SDK()
{
    disconnect();
}

// ─────────────────────────────────────────────────────────────────────────────
// Helpers de nó GenICam
//
// A API do DMV-SDK opera em dois níveis:
//   Nível 1 (simples): DcNodeListGetValue / DcNodeListSetValue — tudo como string.
//   Nível 2 (tipado):  DcNodeListGetNode → DcNodeCastToXxxNode → XxxNodeGetValue.
//
// Funções como DcNodeListGetFloatValue / DcNodeListGetIntValue / DcNodeListExecute
// NÃO EXISTEM no SDK. Sempre use o padrão GetNode → Cast → operação tipada.
// ─────────────────────────────────────────────────────────────────────────────

std::string ImplCameraComponent_DMV_SDK::readNodeStr(const char* nodeName) const
{
    if (!m_nodelist) return "N/A";
    char buf[DMV_NODE_BUF_SIZE] = {};
    size_t size = sizeof(buf);
    if (DcNodeListGetValue(m_nodelist, nodeName, buf, &size) == DC_ERROR_SUCCESS)
        return std::string(buf);
    return "N/A";
}

double ImplCameraComponent_DMV_SDK::readNodeFloat(const char* nodeName) const
{
    if (!m_nodelist) return 0.0;

    DcNode node = nullptr;
    if (DcNodeListGetNode(m_nodelist, nodeName, &node) != DC_ERROR_SUCCESS || !node)
        return 0.0;

    DcFloatNode fnode = DcNodeCastToFloatNode(node);
    if (!fnode) return 0.0;

    double value = 0.0;
    DcFloatNodeGetValue(fnode, &value);
    return value;
}

int64_t ImplCameraComponent_DMV_SDK::readNodeInt(const char* nodeName) const
{
    if (!m_nodelist) return 0;

    DcNode node = nullptr;
    if (DcNodeListGetNode(m_nodelist, nodeName, &node) != DC_ERROR_SUCCESS || !node)
        return 0;

    DcIntegerNode inode = DcNodeCastToIntegerNode(node);
    if (!inode) return 0;

    int64_t value = 0;
    DcIntegerNodeGetValue(inode, &value);
    return value;
}

// Executa um nó de comando (ex: UserSetLoad, UserSetSave, AcquisitionStart).
// Padrão obrigatório: GetNode → CastToCommandNode → Execute.
// DcNodeListExecute() NÃO EXISTE no SDK.
CameraStatus ImplCameraComponent_DMV_SDK::executeNode(const char* nodeName)
{
    if (!m_nodelist) return CameraStatus::NotConnected;

    DcNode node = nullptr;
    if (DcNodeListGetNode(m_nodelist, nodeName, &node) != DC_ERROR_SUCCESS || !node)
    {
        Falcor::Logger.log_error("[DMV] executeNode: nó '{}' não encontrado.", nodeName);
        return CameraStatus::InternalError;
    }

    DcCommandNode cmd = DcNodeCastToCommandNode(node);
    if (!cmd)
    {
        Falcor::Logger.log_error("[DMV] executeNode: '{}' não é um CommandNode.", nodeName);
        return CameraStatus::InternalError;
    }

    if (DcCommandNodeExecute(cmd) != DC_ERROR_SUCCESS)
    {
        Falcor::Logger.log_error("[DMV] executeNode: falha ao executar '{}'.", nodeName);
        return CameraStatus::InternalError;
    }

    return CameraStatus::Ok;
}

const char* ImplCameraComponent_DMV_SDK::presetToStr(DeltaUserPreset preset)
{
    switch (preset) {
        case DeltaUserPreset::Default:  return "Default";
        case DeltaUserPreset::UserSet1: return "UserSet1";
        case DeltaUserPreset::UserSet2: return "UserSet2";
        case DeltaUserPreset::UserSet3: return "UserSet3";
        case DeltaUserPreset::UserSet4: return "UserSet4";
        default:                        return "Default";
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Connect
// ─────────────────────────────────────────────────────────────────────────────

CameraStatus ImplCameraComponent_DMV_SDK::connect(const CameraConfig& config)
{
    if (m_connected.load())
        return CameraStatus::AlreadyConnected;

    m_config = config;

    if (DcSystemCreate(&m_system) != DC_ERROR_SUCCESS)
    {
        Falcor::Logger.log_error("[DMV] connect: falha ao criar DcSystem.");
        return CameraStatus::InternalError;
    }

    // DcSystemGetDevice requer DcDeviceHint*, NÃO uma const char* direto.
    // Use as funções de fábrica: DcDeviceIPHint, DcDeviceSerialNumberHint, etc.
    const std::string& ip = m_config.protocol.GigE.camera_ip;
    DcDeviceHint hint = DcDeviceIPHint(ip.c_str());

    if (DcSystemGetDevice(m_system, &hint, &m_device) != DC_ERROR_SUCCESS)
    {
        Falcor::Logger.log_error("[DMV] connect: câmera com IP '{}' não encontrada.", ip);
        DcSystemDestroy(m_system);
        m_system = nullptr;
        return CameraStatus::NotConnected;
    }

    if (DcDeviceOpen(m_device, DC_DEVICE_ACCESS_TYPE_CONTROL) != DC_ERROR_SUCCESS)
    {
        Falcor::Logger.log_error("[DMV] connect: falha ao abrir dispositivo (IP={}).", ip);
        DcSystemDestroy(m_system);
        m_system = nullptr;
        return CameraStatus::InternalError;
    }

    if (DcDeviceGetRemoteNodeList(m_device, &m_nodelist) != DC_ERROR_SUCCESS)
    {
        Falcor::Logger.log_error("[DMV] connect: falha ao obter nodelist remoto.");
        DcDeviceClose(m_device);
        DcSystemDestroy(m_system);
        m_system = nullptr;
        m_device = nullptr;
        return CameraStatus::InternalError;
    }

    if (DcDeviceGetDataStream(m_device, 0, &m_stream) != DC_ERROR_SUCCESS)
    {
        Falcor::Logger.log_error("[DMV] connect: falha ao obter DataStream.");
        DcDeviceClose(m_device);
        DcSystemDestroy(m_system);
        m_system = nullptr;
        m_device = nullptr;
        m_nodelist = nullptr;
        return CameraStatus::InternalError;
    }

    m_connected.store(true);
    logCameraInfo();

    return CameraStatus::Ok;
}

// ─────────────────────────────────────────────────────────────────────────────
// Start Stream
// ─────────────────────────────────────────────────────────────────────────────

CameraStatus ImplCameraComponent_DMV_SDK::startStream()
{
    if (!m_connected.load())
        return CameraStatus::NotConnected;

    if (m_streaming.load())
        return CameraStatus::AlreadyStreaming;

    // Modo de aquisição contínuo
    if (DcNodeListSetValue(m_nodelist, "AcquisitionMode", "Continuous") != DC_ERROR_SUCCESS)
    {
        Falcor::Logger.log_error("[DMV] startStream: falha ao definir AcquisitionMode=Continuous.");
        return CameraStatus::InternalError;
    }

    // Desabilita trigger de hardware — duas chamadas separadas.
    // DcNodeListSetSelectedValue() NÃO EXISTE no SDK.
    if (DcNodeListSetValue(m_nodelist, "TriggerSelector", "FrameStart") != DC_ERROR_SUCCESS)
        Falcor::Logger.log_warning("[DMV] startStream: TriggerSelector não disponível (ignorado).");

    if (DcNodeListSetValue(m_nodelist, "TriggerMode", "Off") != DC_ERROR_SUCCESS)
        Falcor::Logger.log_warning("[DMV] startStream: TriggerMode não disponível (ignorado).");

    // Aloca e anuncia buffers de DMA
    m_buffers.clear();
    for (uint32_t i = 0; i < DMV_BUFFER_COUNT; i++)
    {
        DcBuffer buffer = nullptr;
        if (DcDataStreamAllocAndAnnounceBuffer(m_stream, nullptr, &buffer) != DC_ERROR_SUCCESS)
        {
            Falcor::Logger.log_error("[DMV] startStream: falha ao alocar buffer #{}.", i);
            // Revoga buffers já alocados antes de retornar
            for (DcBuffer b : m_buffers)
                DcDataStreamRevokeBuffer(m_stream, b, nullptr, nullptr);
            m_buffers.clear();
            return CameraStatus::InternalError;
        }
        if (DcDataStreamQueueBuffer(m_stream, buffer) != DC_ERROR_SUCCESS)
        {
            Falcor::Logger.log_error("[DMV] startStream: falha ao enfileirar buffer #{}.", i);
            for (DcBuffer b : m_buffers)
                DcDataStreamRevokeBuffer(m_stream, b, nullptr, nullptr);
            m_buffers.clear();
            return CameraStatus::InternalError;
        }
        m_buffers.push_back(buffer);
    }

    if (DcDataStreamStartAcquisition(m_stream) != DC_ERROR_SUCCESS)
    {
        Falcor::Logger.log_error("[DMV] startStream: falha ao iniciar aquisição.");
        for (DcBuffer b : m_buffers)
            DcDataStreamRevokeBuffer(m_stream, b, nullptr, nullptr);
        m_buffers.clear();
        return CameraStatus::InternalError;
    }

    m_streaming.store(true);
    Falcor::Logger.log_info("[DMV] Streaming iniciado ({} buffers alocados).", DMV_BUFFER_COUNT);
    return CameraStatus::Ok;
}

// ─────────────────────────────────────────────────────────────────────────────
// Get Frame
// ─────────────────────────────────────────────────────────────────────────────

CameraStatus ImplCameraComponent_DMV_SDK::getRawFrame(RawFrame& outFrame)
{
    if (!m_streaming.load())
        return CameraStatus::NotStreaming;

    DcBuffer buffer = nullptr;
    DcError error = DcDataStreamGetFilledBuffer(m_stream, DMV_IMAGE_WAIT_MS, &buffer);

    if (error == DC_ERROR_TIMEOUT)  return CameraStatus::Timeout;
    if (error != DC_ERROR_SUCCESS)  return CameraStatus::InternalError;

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

    void*  data_ptr = nullptr;
    size_t width    = 0;
    size_t height   = 0;
    size_t dataSize = 0;

    if (DcImageGetDataPtr (image, &data_ptr) != DC_ERROR_SUCCESS || !data_ptr ||
        DcImageGetWidth   (image, &width)    != DC_ERROR_SUCCESS ||
        DcImageGetHeight  (image, &height)   != DC_ERROR_SUCCESS ||
        DcImageGetDataSize(image, &dataSize) != DC_ERROR_SUCCESS || dataSize == 0)
    {
        DcDataStreamQueueBuffer(m_stream, buffer);
        return CameraStatus::InternalError;
    }

    outFrame.data.resize(dataSize);
    std::memcpy(outFrame.data.data(), data_ptr, dataSize);
    outFrame.width  = static_cast<uint32_t>(width);
    outFrame.height = static_cast<uint32_t>(height);
    outFrame.format = m_config.format;
    outFrame.metadata.frameId = ++m_frameCounter;

    DcDataStreamQueueBuffer(m_stream, buffer);
    return CameraStatus::Ok;
}

// ─────────────────────────────────────────────────────────────────────────────
// Stop Stream
// ─────────────────────────────────────────────────────────────────────────────

void ImplCameraComponent_DMV_SDK::stopStream()
{
    if (!m_streaming.load()) return;

    DcDataStreamStopAcquisition(m_stream, false);

    // Revoga buffers antes de limpar — evita leak de memória interna do SDK
    for (DcBuffer buf : m_buffers)
        DcDataStreamRevokeBuffer(m_stream, buf, nullptr, nullptr);
    m_buffers.clear();

    m_streaming.store(false);
    Falcor::Logger.log_info("[DMV] Streaming encerrado.");
}

// ─────────────────────────────────────────────────────────────────────────────
// Disconnect
// ─────────────────────────────────────────────────────────────────────────────

void ImplCameraComponent_DMV_SDK::disconnect()
{
    stopStream();

    if (m_device) {
        DcDeviceClose(m_device);
        m_device = nullptr;
    }
    if (m_system) {
        DcSystemDestroy(m_system);
        m_system = nullptr;
    }

    resetHandles();
    m_connected.store(false);
    Falcor::Logger.log_info("[DMV] Camera desconectada.");
}

// ─────────────────────────────────────────────────────────────────────────────
// User Preset Management
// ─────────────────────────────────────────────────────────────────────────────

CameraStatus ImplCameraComponent_DMV_SDK::loadUserPreset(DeltaUserPreset preset)
{
    if (!m_connected.load()) return CameraStatus::NotConnected;

    const char* presetStr = presetToStr(preset);
    Falcor::Logger.log_info("[DMV] Carregando User Preset: '{}'...", presetStr);

    // 1. Seleciona qual slot carregar
    if (DcNodeListSetValue(m_nodelist, "UserSetSelector", presetStr) != DC_ERROR_SUCCESS)
    {
        Falcor::Logger.log_error("[DMV] loadUserPreset: falha ao selecionar '{}'.", presetStr);
        return CameraStatus::InternalError;
    }

    // 2. Executa o comando UserSetLoad:
    //    GetNode("UserSetLoad") → CastToCommandNode → Execute
    CameraStatus status = executeNode("UserSetLoad");
    if (status != CameraStatus::Ok)
    {
        Falcor::Logger.log_error("[DMV] loadUserPreset: falha ao executar UserSetLoad.");
        return status;
    }

    Falcor::Logger.log_info("[DMV] Preset '{}' carregado com sucesso.", presetStr);
    logCameraInfo();
    return CameraStatus::Ok;
}

CameraStatus ImplCameraComponent_DMV_SDK::saveUserPreset(DeltaUserPreset preset)
{
    if (!m_connected.load()) return CameraStatus::NotConnected;

    // Default é somente leitura — câmera rejeita escrita nele
    if (preset == DeltaUserPreset::Default)
    {
        Falcor::Logger.log_error("[DMV] saveUserPreset: o preset 'Default' é somente leitura.");
        return CameraStatus::InvalidConfiguration;
    }

    const char* presetStr = presetToStr(preset);
    Falcor::Logger.log_info("[DMV] Salvando configuração atual em preset '{}'...", presetStr);

    if (DcNodeListSetValue(m_nodelist, "UserSetSelector", presetStr) != DC_ERROR_SUCCESS)
    {
        Falcor::Logger.log_error("[DMV] saveUserPreset: falha ao selecionar '{}'.", presetStr);
        return CameraStatus::InternalError;
    }

    // Executa UserSetSave:
    //    GetNode("UserSetSave") → CastToCommandNode → Execute
    CameraStatus status = executeNode("UserSetSave");
    if (status != CameraStatus::Ok)
    {
        Falcor::Logger.log_error("[DMV] saveUserPreset: falha ao executar UserSetSave.");
        return status;
    }

    Falcor::Logger.log_info("[DMV] Configurações salvas em '{}' com sucesso.", presetStr);
    return CameraStatus::Ok;
}

CameraStatus ImplCameraComponent_DMV_SDK::setDefaultPreset(DeltaUserPreset preset)
{
    if (!m_connected.load()) return CameraStatus::NotConnected;

    const char* presetStr = presetToStr(preset);
    Falcor::Logger.log_info("[DMV] Definindo preset padrão (boot) como '{}'...", presetStr);

    // UserSetDefault é um nó de enum — basta setá-lo como string
    if (DcNodeListSetValue(m_nodelist, "UserSetDefault", presetStr) != DC_ERROR_SUCCESS)
    {
        Falcor::Logger.log_error("[DMV] setDefaultPreset: falha ao definir UserSetDefault='{}'.", presetStr);
        return CameraStatus::InternalError;
    }

    Falcor::Logger.log_info("[DMV] Preset padrão definido como '{}'. "
        "Será carregado automaticamente ao ligar a câmera.", presetStr);
    return CameraStatus::Ok;
}

// ─────────────────────────────────────────────────────────────────────────────
// Camera Info
// ─────────────────────────────────────────────────────────────────────────────

DeltaCameraInfo ImplCameraComponent_DMV_SDK::queryCameraInfo() const
{
    DeltaCameraInfo info;

    if (!m_connected.load() || !m_nodelist)
        return info;

    // ── Identificação ──────────────────────────────────────────────────────
    info.model        = readNodeStr("DeviceModelName");
    info.serial       = readNodeStr("DeviceSerialNumber");
    info.userID       = readNodeStr("DeviceUserID");
    info.manufacturer = readNodeStr("DeviceVendorName");
    info.firmware     = readNodeStr("DeviceFirmwareVersion");

    // ── Rede GigE Vision ───────────────────────────────────────────────────
    info.ip     = readNodeStr("GevCurrentIPAddress");
    info.mac    = readNodeStr("GevDeviceMACAddress");
    info.subnet = readNodeStr("GevCurrentSubnetMask");

    // Fallback: se o IP GigE não estiver disponível, usa o da configuração
    if (info.ip == "N/A" || info.ip.empty())
        info.ip = m_config.protocol.GigE.camera_ip;

    // ── Configuração de imagem ─────────────────────────────────────────────
    // Float e Integer lidos via GetNode → Cast → typed getter
    info.pixelFormat = readNodeStr("PixelFormat");
    info.width       = static_cast<uint32_t>(readNodeInt("Width"));
    info.height      = static_cast<uint32_t>(readNodeInt("Height"));
    info.fps         = readNodeFloat("AcquisitionFrameRate");
    info.exposureUs  = readNodeFloat("ExposureTime");
    info.gainDb      = readNodeFloat("Gain");

    // Fallbacks da config se os nós não estiverem disponíveis
    if (info.width  == 0) info.width  = m_config.width;
    if (info.height == 0) info.height = m_config.height;
    if (info.fps    == 0) info.fps    = m_config.fps;

    // ── User Preset ativa ──────────────────────────────────────────────────
    info.presetName  = readNodeStr("UserSetSelector");
    if      (info.presetName == "UserSet1") info.activePreset = DeltaUserPreset::UserSet1;
    else if (info.presetName == "UserSet2") info.activePreset = DeltaUserPreset::UserSet2;
    else if (info.presetName == "UserSet3") info.activePreset = DeltaUserPreset::UserSet3;
    else if (info.presetName == "UserSet4") info.activePreset = DeltaUserPreset::UserSet4;
    else                                    info.activePreset = DeltaUserPreset::Default;

    return info;
}

void ImplCameraComponent_DMV_SDK::logCameraInfo() const
{
    const auto info = queryCameraInfo();

    Falcor::Logger.log_info("┌─────────────────────────────────────────────────────────┐");
    Falcor::Logger.log_info("│              DELTA CAMERA — INFORMAÇÕES                 │");
    Falcor::Logger.log_info("├─────────────────────────────────────────────────────────┤");
    Falcor::Logger.log_info("│  Modelo     : {:<44} │", info.model);
    Falcor::Logger.log_info("│  Serial     : {:<44} │", info.serial);
    Falcor::Logger.log_info("│  User ID    : {:<44} │", info.userID);
    Falcor::Logger.log_info("│  Fabricante : {:<44} │", info.manufacturer);
    Falcor::Logger.log_info("│  Firmware   : {:<44} │", info.firmware);
    Falcor::Logger.log_info("├─────────────────────────────────────────────────────────┤");
    Falcor::Logger.log_info("│  IP         : {:<44} │", info.ip);
    Falcor::Logger.log_info("│  MAC        : {:<44} │", info.mac);
    Falcor::Logger.log_info("│  Sub-rede   : {:<44} │", info.subnet);
    Falcor::Logger.log_info("├─────────────────────────────────────────────────────────┤");
    Falcor::Logger.log_info("│  Resolução  : {}x{:<38} │", info.width, info.height);
    Falcor::Logger.log_info("│  Formato    : {:<44} │", info.pixelFormat);
    Falcor::Logger.log_info("│  FPS        : {:<44.2f} │", info.fps);
    Falcor::Logger.log_info("│  Exposure   : {:.1f} us{:<39} │", info.exposureUs, "");
    Falcor::Logger.log_info("│  Ganho      : {:.2f} dB{:<39} │", info.gainDb, "");
    Falcor::Logger.log_info("├─────────────────────────────────────────────────────────┤");
    Falcor::Logger.log_info("│  User Preset: {:<44} │", info.presetName);
    Falcor::Logger.log_info("└─────────────────────────────────────────────────────────┘");
}

// ─────────────────────────────────────────────────────────────────────────────
// Enumeração de câmeras — varredura de rede via interface-based API
//
// O SDK NÃO possui DcSystemUpdateDeviceList / DcSystemGetDeviceCount /
// DcSystemGetDeviceByIndex.
//
// O fluxo correto conforme o sample device_enumeration/source/main.c:
//   System → UpdateInterfaceList → GetInterface → InterfaceOpen
//         → InterfaceUpdateDeviceList → InterfaceGetDevice
//
// IP e MAC são lidos via nodelist da interface-pai com DeviceSelector.
// ─────────────────────────────────────────────────────────────────────────────

std::vector<DeltaDiscoveredCamera>
ImplCameraComponent_DMV_SDK::enumerateCameras(uint32_t timeoutMs)
{
    std::vector<DeltaDiscoveredCamera> result;

    DcSystem system = nullptr;
    if (DcSystemCreate(&system) != DC_ERROR_SUCCESS)
    {
        Falcor::Logger.log_error("[DMV] enumerateCameras: falha ao criar DcSystem.");
        return result;
    }

    // 1. Descobre interfaces de rede disponíveis
    if (DcSystemUpdateInterfaceList(system, nullptr, DC_INFINITE) != DC_ERROR_SUCCESS)
    {
        Falcor::Logger.log_warning("[DMV] enumerateCameras: DcSystemUpdateInterfaceList falhou.");
        DcSystemDestroy(system);
        return result;
    }

    size_t intf_count = 0;
    DcSystemGetInterfaceCount(system, &intf_count);

    for (size_t i = 0; i < intf_count; ++i)
    {
        DcInterface intf = nullptr;
        if (DcSystemGetInterface(system, i, &intf) != DC_ERROR_SUCCESS)
            continue;

        // Interface precisa estar aberta antes de atualizar lista de dispositivos
        if (DcInterfaceOpen(intf) != DC_ERROR_SUCCESS)
            continue;

        // 2. Descobre dispositivos nessa interface
        DcInterfaceUpdateDeviceList(intf, nullptr, static_cast<uint64_t>(timeoutMs));

        size_t dev_count = 0;
        DcInterfaceGetDeviceCount(intf, &dev_count);

        // Nodelist da interface permite ler IP/MAC via DeviceSelector
        DcNodeList intf_nodelist = nullptr;
        DcInterfaceGetNodeList(intf, &intf_nodelist);

        // Obtém o nó DeviceSelector (IntegerNode) para selecionar o dispositivo
        DcIntegerNode device_selector = nullptr;
        if (intf_nodelist)
        {
            DcNode selector_node = nullptr;
            if (DcNodeListGetNode(intf_nodelist, "DeviceSelector", &selector_node) == DC_ERROR_SUCCESS
                && selector_node)
            {
                device_selector = DcNodeCastToIntegerNode(selector_node);
            }
        }

        for (size_t j = 0; j < dev_count; ++j)
        {
            DcDevice device = nullptr;
            if (DcInterfaceGetDevice(intf, j, &device) != DC_ERROR_SUCCESS)
                continue;

            DeltaDiscoveredCamera cam;
            char buf[DMV_NODE_BUF_SIZE] = {};
            size_t sz;

            // Informações básicas via DcDeviceGetInfo (não requer DcDeviceOpen)
            sz = sizeof(buf);
            if (DcDeviceGetInfo(device, DC_DEVICE_INFO_VENDOR, buf, &sz) == DC_ERROR_SUCCESS)
                cam.model = buf;  // temporário; substituído abaixo se DC_DEVICE_INFO_MODEL disponível
            sz = sizeof(buf);
            if (DcDeviceGetInfo(device, DC_DEVICE_INFO_MODEL, buf, &sz) == DC_ERROR_SUCCESS)
                cam.model = buf;
            sz = sizeof(buf);
            if (DcDeviceGetInfo(device, DC_DEVICE_INFO_SERIAL_NUMBER, buf, &sz) == DC_ERROR_SUCCESS)
                cam.serial = buf;

            // IP e MAC via nodelist da interface-pai, com DeviceSelector apontando para este índice
            if (intf_nodelist && device_selector)
            {
                DcIntegerNodeSetValue(device_selector, static_cast<int64_t>(j));

                sz = sizeof(buf);
                if (DcNodeListGetValue(intf_nodelist, "GevDeviceIPAddress", buf, &sz) == DC_ERROR_SUCCESS)
                    cam.ip = buf;
                sz = sizeof(buf);
                if (DcNodeListGetValue(intf_nodelist, "GevDeviceMACAddress", buf, &sz) == DC_ERROR_SUCCESS)
                    cam.mac = buf;
            }

            result.push_back(std::move(cam));
        }

        DcInterfaceClose(intf);
    }

    DcSystemDestroy(system);

    if (result.empty())
        Falcor::Logger.log_info("[DMV] enumerateCameras: nenhuma câmera encontrada na rede.");
    else
        Falcor::Logger.log_info("[DMV] enumerateCameras: {} câmera(s) encontrada(s).", result.size());

    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// Feature Stream — salvar/carregar configurações GenICam em arquivo .dfs
// ─────────────────────────────────────────────────────────────────────────────

CameraStatus ImplCameraComponent_DMV_SDK::saveFeatureStreamToFile(const std::string& filepath) const
{
    if (!m_connected.load() || !m_nodelist)
        return CameraStatus::NotConnected;

    DcError err = DcNodeListSaveFeatureStream(m_nodelist, filepath.c_str());
    if (err != DC_ERROR_SUCCESS)
    {
        Falcor::Logger.log_error("[DMV] saveFeatureStream: falha ao salvar '{}' (err={}).",
            filepath, static_cast<int>(err));
        return CameraStatus::InternalError;
    }

    Falcor::Logger.log_info("[DMV] saveFeatureStream: configurações salvas em '{}'.", filepath);
    return CameraStatus::Ok;
}

CameraStatus ImplCameraComponent_DMV_SDK::loadFeatureStreamFromFile(const std::string& filepath)
{
    if (!m_connected.load() || !m_nodelist)
        return CameraStatus::NotConnected;

    char error_message[512] = {};
    size_t error_message_size = sizeof(error_message);

    DcError err = DcNodeListLoadFeatureStream(
        m_nodelist, filepath.c_str(), error_message, &error_message_size);

    if (err != DC_ERROR_SUCCESS)
    {
        Falcor::Logger.log_error("[DMV] loadFeatureStream: falha ao carregar '{}' (err={}).",
            filepath, static_cast<int>(err));
        if (error_message[0] != '\0')
            Falcor::Logger.log_error("[DMV] loadFeatureStream detalhes:\n{}", error_message);
        return CameraStatus::InternalError;
    }

    Falcor::Logger.log_info("[DMV] loadFeatureStream: configurações carregadas de '{}'.", filepath);
    return CameraStatus::Ok;
}

// ─────────────────────────────────────────────────────────────────────────────
// Getters
// ─────────────────────────────────────────────────────────────────────────────

bool        ImplCameraComponent_DMV_SDK::isConnected() const { return m_connected.load(); }
bool        ImplCameraComponent_DMV_SDK::isStreaming() const { return m_streaming.load(); }
uint32_t    ImplCameraComponent_DMV_SDK::getWidth()    const { return m_config.width; }
uint32_t    ImplCameraComponent_DMV_SDK::getHeight()   const { return m_config.height; }
PixelFormat ImplCameraComponent_DMV_SDK::getPixelFormat() const { return m_config.format; }
const char* ImplCameraComponent_DMV_SDK::getCameraName()  const { return "Delta GigE Vision (DMV-SDK)"; }

// ─────────────────────────────────────────────────────────────────────────────
// Reset Handles
// ─────────────────────────────────────────────────────────────────────────────

void ImplCameraComponent_DMV_SDK::resetHandles() noexcept
{
    m_system   = nullptr;
    m_device   = nullptr;
    m_stream   = nullptr;
    m_nodelist = nullptr;
}
