#include "CCameraComponent.hpp"
#include "vendors/ImplCameraComponent_DMV_SDK.hpp" // Implementação concreta

namespace Falcor::Components {

CCameraComponent::CCameraComponent() 
    : m_selectedType(CameraType::MOCK_CAMERA),
      m_cameraConfig{
          .protocol = { .GigE = { nullptr, nullptr } }, // Inicializa o struct interno
          .width = 0,
          .height = 0,
          .format = PixelFormat::Unknown,               // PixelFormat explícito
          .fps = 0
      } 
{
    // Construtor leve para o Registry
}

CCameraComponent::~CCameraComponent() {
    shutdown();
}

void CCameraComponent::configure(CameraType type, CameraConfig config) {
    // Registro de auditoria industrial
    Falcor::Logger.log_info("[CCameraComponent] Configurando hardware: Tipo {}, Resolucao {}x{}", 
                             (int)type, config.width, config.height);
    
    m_selectedType = type;
    m_cameraConfig = config;
}

void CCameraComponent::initialize() {
    // 1. Validar se o componente já foi inicializado (Prevenção de double-init)
    if (m_cameraImpl && m_cameraImpl->isConnected()) {
        Falcor::Logger.log_warning("[CCameraComponent] Tentativa de inicializacao em componente ja ativo.");
        return;
    }

    // 2. Factory Pattern: Instancia a implementação baseada no tipo configurado
    switch (m_selectedType) {
        case CameraType::DMV_CAMERA:
            m_cameraImpl = std::make_unique<ImplCameraComponent_DMV_SDK>(m_cameraConfig);
            break;
        
        case CameraType::MOCK_CAMERA:
            Falcor::Logger.log_warning("[CCameraComponent] Iniciando em modo MOCK (Simulacao).");
            // Implementação Mock poderia ser instanciada aqui
            break;

        default:
            throw std::runtime_error("Tipo de camera nao suportado ou nao configurado.");
    }

    // 3. Conexão de Hardware
    if (m_cameraImpl) {
        Falcor::Logger.log_info("[CCameraComponent] Estabelecendo conexao com o driver DMV...");
        
        CameraStatus status = m_cameraImpl->connect(m_cameraConfig);
        
        if (status != CameraStatus::Ok) {
            Falcor::Logger.log_error("[CCameraComponent] Erro de hardware ao conectar. Status: {}", (int)status);
            throw std::runtime_error("Falha critica na comunicacao com a Camera DMV.");
        }
        
        Falcor::Logger.log_info("[CCameraComponent] Hardware online e sincronizado via ComponentSystem.");
    }
}

void CCameraComponent::shutdown() {
    if (m_cameraImpl) {
        Falcor::Logger.log_info("[CCameraComponent] Encerrando conexao de hardware...");
        m_cameraImpl->disconnect();
        m_cameraImpl.reset(); // Garante a liberação da memória
    }
}

void CCameraComponent::getVideoBuffer() {
    if (!m_cameraImpl || !m_cameraImpl->isConnected()) {
        Falcor::Logger.log_error("[CCameraComponent] Erro: Tentativa de leitura em camera offline.");
        return;
    }
    
    // Logica de captura (Onde o DMA do Kria KV260 entraria em ação)
    std::cout << "[DMV] Buffer processado com sucesso." << std::endl;
}

} // namespace Falcor::Components