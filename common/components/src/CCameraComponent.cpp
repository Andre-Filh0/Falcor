#include "CCameraComponent.hpp"

#ifdef FALCOR_HAVE_DELTA_SDK
    #include "vendors/ImplCameraComponent_DMV_SDK.hpp"
#endif
#include "vendors/ImplCameraComponent_Mock.hpp"

namespace Falcor::Components {

CCameraComponent::CCameraComponent() = default;

// BUG FIX #8: Destrutor NAO chama shutdown().
// ComponentRegistry::shutdown_all() ja chama shutdown() em todos os componentes.
CCameraComponent::~CCameraComponent() = default;

void CCameraComponent::configure(CameraType type, CameraConfig config)
{
    Falcor::Logger.log_info(
        "[CCameraComponent] Configurando camera. Type={}, Resolucao={}x{}",
        static_cast<int>(type),
        config.width,
        config.height
    );

    m_selectedType = type;
    m_cameraConfig = config;
}

void CCameraComponent::initialize()
{
    if (m_cameraImpl && m_cameraImpl->isConnected())
        return;

    // Instancia a implementacao correta
    switch (m_selectedType)
    {
        case CameraType::DMV_CAMERA:
#ifdef FALCOR_HAVE_DELTA_SDK
            m_cameraImpl = std::make_unique<ImplCameraComponent_DMV_SDK>(m_cameraConfig);
#else
            throw std::runtime_error(
                "CCameraComponent: DMV_CAMERA solicitado mas o projeto foi compilado "
                "sem o Delta SDK (FALCOR_USE_DELTA_SDK=OFF). "
                "Use o preset 'linux-kria-release' ou force MOCK_CAMERA para testes no PC.");
#endif
            break;

        case CameraType::MOCK_CAMERA:
            m_cameraImpl = std::make_unique<ImplCameraComponent_Mock>(m_cameraConfig);
            break;

        default:
            throw std::runtime_error("CCameraComponent: tipo de camera nao suportado.");
    }

    // Tentativas de conexao com retry
    CameraStatus connectStatus = CameraStatus::InternalError;

    for (int attempt = 1; attempt <= kMaxConnectRetries; ++attempt)
    {
        Falcor::Logger.log_info(
            "[CCameraComponent] Tentativa de conexao {}/{}...", attempt, kMaxConnectRetries);

        connectStatus = m_cameraImpl->connect(m_cameraConfig);

        if (connectStatus == CameraStatus::Ok)
        {
            Falcor::Logger.log_info("[CCameraComponent] Camera '{}' conectada.",
                m_cameraImpl->getCameraName());
            break;
        }

        Falcor::Logger.log_warning(
            "[CCameraComponent] Falha na tentativa {}/{} (status={}). "
            "Aguardando {}ms antes de tentar novamente...",
            attempt, kMaxConnectRetries,
            static_cast<int>(connectStatus),
            kRetryDelayMs);

        if (attempt < kMaxConnectRetries)
            std::this_thread::sleep_for(std::chrono::milliseconds(kRetryDelayMs));
    }

    if (connectStatus != CameraStatus::Ok)
        throw std::runtime_error(
            "CCameraComponent: camera nao respondeu apos todas as tentativas de conexao.");

    // Iniciar stream
    CameraStatus streamStatus = m_cameraImpl->startStream();
    if (streamStatus != CameraStatus::Ok)
        throw std::runtime_error("CCameraComponent: falha ao iniciar stream da camera.");

    Falcor::Logger.log_info("[CCameraComponent] Camera inicializada e streaming.");
}

void CCameraComponent::shutdown()
{
    if (m_cameraImpl)
    {
        Falcor::Logger.log_info("[CCameraComponent] Encerrando camera...");
        m_cameraImpl->disconnect();
        m_cameraImpl.reset();
    }
}

void CCameraComponent::getVideoBuffer()
{
    if (!m_cameraImpl || !m_cameraImpl->isStreaming())
    {
        Falcor::Logger.log_warning("[CCameraComponent] Camera nao esta em streaming.");
        return;
    }

    RawFrame frame;
    CameraStatus status = m_cameraImpl->getRawFrame(frame);

    if (status == CameraStatus::Ok)
    {
        Falcor::Logger.log_info(
            "[CCameraComponent] Frame #{} recebido: {} bytes, {}x{}",
            frame.metadata.frameId,
            frame.data.size(),
            frame.width,
            frame.height
        );
    }
    else if (status == CameraStatus::Timeout)
    {
        Falcor::Logger.log_warning("[CCameraComponent] Timeout ao capturar frame.");
    }
    else
    {
        Falcor::Logger.log_error("[CCameraComponent] Falha ao capturar frame (status={}).",
            static_cast<int>(status));
    }
}

} // namespace Falcor::Components
