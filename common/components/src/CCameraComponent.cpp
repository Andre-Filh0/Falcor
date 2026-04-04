#include "CCameraComponent.hpp"

#ifdef FALCOR_HAVE_DELTA_SDK
    #include "vendors/ImplCameraComponent_DMV_SDK.hpp"
#endif
#include "vendors/ImplCameraComponent_Mock.hpp"

namespace Falcor::Components {

CCameraComponent::CCameraComponent()  = default;
CCameraComponent::~CCameraComponent() = default;

void CCameraComponent::configure(CameraType type, CameraConfig config)
{
    Falcor::Logger.log_info(
        "[CCameraComponent] Configurando camera. Type={}, {}x{} @ {} fps",
        static_cast<int>(type), config.width, config.height, config.fps);

    m_selectedType = type;
    m_cameraConfig = config;
}

// ─────────────────────────────────────────────────────────────────────────────
// initialize — retry infinito até câmera conectar
// ─────────────────────────────────────────────────────────────────────────────

void CCameraComponent::initialize()
{
    if (m_cameraImpl && m_cameraImpl->isConnected())
        return;

    // Instancia a implementação correta
    switch (m_selectedType)
    {
        case CameraType::DMV_CAMERA:
#ifdef FALCOR_HAVE_DELTA_SDK
            m_cameraImpl = std::make_unique<ImplCameraComponent_DMV_SDK>(m_cameraConfig);
#else
            throw std::runtime_error(
                "CCameraComponent: DMV_CAMERA solicitado mas o projeto foi compilado "
                "sem o Delta SDK (FALCOR_USE_DELTA_SDK=OFF). "
                "Use MOCK_CAMERA para testes ou o preset 'linux-kria-release' na Kria.");
#endif
            break;

        case CameraType::MOCK_CAMERA:
            m_cameraImpl = std::make_unique<ImplCameraComponent_Mock>(m_cameraConfig);
            break;

        default:
            throw std::runtime_error("CCameraComponent: tipo de camera nao suportado.");
    }

    // ── Loop de descoberta e conexão ─────────────────────────────────────────
    // Tenta conectar indefinidamente a cada kRetryIntervalMs.
    // Para sistemas embarcados, é esperado que o operador ligue a câmera
    // antes ou logo após ligar o sistema. O pipeline não inicia sem a câmera.

    int attempt = 0;

    while (true)
    {
        ++attempt;

        // Antes de tentar a conexão por IP, faz uma varredura de rede
        // para descobrir câmeras disponíveis e logar o que foi encontrado.
#ifdef FALCOR_HAVE_DELTA_SDK
        if (m_selectedType == CameraType::DMV_CAMERA)
        {
            Falcor::Logger.log_info(
                "[CCameraComponent] Tentativa #{} — Varrendo rede em busca de cameras Delta...",
                attempt);

            auto found = ImplCameraComponent_DMV_SDK::enumerateCameras();
            if (found.empty())
            {
                Falcor::Logger.log_warning(
                    "[CCameraComponent] Nenhuma camera Delta encontrada na rede. "
                    "Verifique cabos e alimentacao na porta {}.",
                    m_cameraConfig.protocol.GigE.interface_name);
            }
            else
            {
                Falcor::Logger.log_info(
                    "[CCameraComponent] {} camera(s) visivel(is) na rede:", found.size());
                for (const auto& cam : found)
                {
                    Falcor::Logger.log_info(
                        "   Modelo: {}  |  Serial: {}  |  IP: {}  |  MAC: {}",
                        cam.model, cam.serial, cam.ip, cam.mac);
                }
            }
        }
#endif

        // Tentativa de conexão pelo IP configurado
        Falcor::Logger.log_info(
            "[CCameraComponent] Tentativa #{} — Conectando em {}...",
            attempt, m_cameraConfig.protocol.GigE.camera_ip);

        CameraStatus connectStatus = m_cameraImpl->connect(m_cameraConfig);

        if (connectStatus == CameraStatus::Ok)
        {
            Falcor::Logger.log_info("[CCameraComponent] Camera '{}' conectada com sucesso!",
                m_cameraImpl->getCameraName());
            break; // sai do loop
        }

        // Falha: loga o motivo e aguarda antes de tentar de novo
        Falcor::Logger.log_warning(
            "[CCameraComponent] Tentativa #{} falhou (status={}). "
            "Proxima tentativa em {} segundos...",
            attempt,
            static_cast<int>(connectStatus),
            kRetryIntervalMs / 1000);

        std::this_thread::sleep_for(std::chrono::milliseconds(kRetryIntervalMs));
    }

    // ── Inicia o stream após conexão ─────────────────────────────────────────
    CameraStatus streamStatus = m_cameraImpl->startStream();
    if (streamStatus != CameraStatus::Ok)
        throw std::runtime_error("CCameraComponent: falha ao iniciar stream da camera.");

    Falcor::Logger.log_info("[CCameraComponent] Camera inicializada. Pipeline pronto.");
}

// ─────────────────────────────────────────────────────────────────────────────
// shutdown
// ─────────────────────────────────────────────────────────────────────────────

void CCameraComponent::shutdown()
{
    if (m_cameraImpl) {
        Falcor::Logger.log_info("[CCameraComponent] Encerrando camera...");
        m_cameraImpl->disconnect();
        m_cameraImpl.reset();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// getVideoBuffer
// ─────────────────────────────────────────────────────────────────────────────

std::optional<RawFrame> CCameraComponent::getVideoBuffer()
{
    if (!m_cameraImpl || !m_cameraImpl->isStreaming())
    {
        Falcor::Logger.log_warning("[CCameraComponent] Camera nao esta em streaming.");
        return std::nullopt;
    }

    RawFrame frame;
    CameraStatus status = m_cameraImpl->getRawFrame(frame);

    if (status == CameraStatus::Ok)
        return frame;

    if (status == CameraStatus::Timeout)
        Falcor::Logger.log_warning("[CCameraComponent] Timeout ao capturar frame.");
    else
        Falcor::Logger.log_error("[CCameraComponent] Falha ao capturar frame (status={}).",
            static_cast<int>(status));

    return std::nullopt;
}

// ─────────────────────────────────────────────────────────────────────────────
// getDeltaImpl — acesso à API específica Delta
// ─────────────────────────────────────────────────────────────────────────────

ImplCameraComponent_DMV_SDK* CCameraComponent::getDeltaImpl() const
{
#ifdef FALCOR_HAVE_DELTA_SDK
    if (m_selectedType == CameraType::DMV_CAMERA && m_cameraImpl)
        return static_cast<ImplCameraComponent_DMV_SDK*>(m_cameraImpl.get());
#endif
    return nullptr;
}

} // namespace Falcor::Components
