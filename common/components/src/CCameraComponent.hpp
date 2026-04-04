#pragma once
#include <stdexcept>
#include <chrono>
#include <thread>
#include <optional>
#include "ComponentSystem.hpp"
#include "Logger.hpp"
#include "interfaces/ICameraComponent.hpp"

// Avança a declaração — evita incluir o header completo do SDK aqui.
// Quem precisar usar a API específica Delta inclui ImplCameraComponent_DMV_SDK.hpp.
class ImplCameraComponent_DMV_SDK;

namespace Falcor::Components {

/**
 * @brief Componente de câmera registrado no ComponentRegistry.
 *
 * Abstrai a câmera física escolhida (Delta DMV, USB genérica, Mock).
 * Gerencia a conexão com retry automático: se a câmera não for encontrada,
 * continua procurando a cada kRetryIntervalMs até conectar ou o sistema
 * ser encerrado.
 *
 * ATENÇÃO: initialize() bloqueia até a câmera ser encontrada.
 * Em sistemas embarcados, isso é o comportamento esperado — o sistema
 * aguarda a câmera estar pronta antes de iniciar o pipeline.
 *
 * Fluxo de uso:
 * @code
 *   auto* cam = CreateComponent<CCameraComponent>(
 *       CCameraComponent::CameraType::DMV_CAMERA, config);
 *   // initialize() retorna após conexão e stream iniciados.
 *
 *   while (true) {
 *       auto frame = cam->getVideoBuffer();
 *       if (frame) pipe->pushFrame(std::move(*frame));
 *   }
 * @endcode
 */
class CCameraComponent : public CComponent<CCameraComponent> {
public:
    enum class CameraType {
        DMV_CAMERA,          // Delta GigE Vision (DMV-SDK)
        USB_GENERIC_CAMERA,  // USB genérica (futuro)
        MOCK_CAMERA          // câmera simulada para testes
    };

    // Intervalo entre tentativas de conexão (ms).
    // 5 segundos — dá tempo ao operador de checar cabos ou ligar a câmera.
    static constexpr int kRetryIntervalMs = 5000;

    CCameraComponent();
    ~CCameraComponent() override;

    void configure(CameraType type, CameraConfig config);

    void initialize() override;
    void shutdown()   override;

    /**
     * @brief Captura um frame da câmera.
     * @return Frame capturado, ou std::nullopt em caso de timeout/erro.
     */
    std::optional<RawFrame> getVideoBuffer();

    /**
     * @brief Retorna ponteiro para a implementação Delta SDK, se ativa.
     *
     * Permite acessar funcionalidades específicas da câmera Delta:
     * loadUserPreset(), saveUserPreset(), queryCameraInfo(), etc.
     * Retorna nullptr se a implementação atual não for Delta
     * (ex: MOCK_CAMERA ou USB_GENERIC_CAMERA).
     *
     * Uso típico (em src/Camera.cpp):
     * @code
     *   auto* cam = GetComponent<CCameraComponent>();
     *   auto* delta = cam ? cam->getDeltaImpl() : nullptr;
     *   if (delta) delta->loadUserPreset(DeltaUserPreset::UserSet1);
     * @endcode
     */
    ImplCameraComponent_DMV_SDK* getDeltaImpl() const;

private:
    CameraType m_selectedType = CameraType::DMV_CAMERA;
    CameraConfig m_cameraConfig{};
    std::unique_ptr<ICameraComponent> m_cameraImpl;
};

} // namespace Falcor::Components
