#pragma once
#include <stdexcept>
#include <chrono>
#include <thread>
#include <optional>
#include "ComponentSystem.hpp"
#include "Logger.hpp"
#include "interfaces/ICameraComponent.hpp"

namespace Falcor::Components {

class CCameraComponent : public CComponent<CCameraComponent> {
public:
    enum class CameraType {
        DMV_CAMERA,
        USB_GENERIC_CAMERA,
        MOCK_CAMERA
    };

    // Tentativas de reconexao e intervalo entre elas
    static constexpr int kMaxConnectRetries  = 5;
    static constexpr int kRetryDelayMs       = 2000;

    CCameraComponent();
    ~CCameraComponent() override;

    void configure(CameraType type, CameraConfig config);

    void initialize() override;
    void shutdown() override;

    /**
     * @brief Captura um frame da camera.
     * @return Frame capturado, ou std::nullopt em caso de timeout ou erro.
     */
    std::optional<RawFrame> getVideoBuffer();

private:
    // Padrao: DMV. Mock so e usado quando explicitamente solicitado.
    CameraType m_selectedType = CameraType::DMV_CAMERA;
    CameraConfig m_cameraConfig{};
    std::unique_ptr<ICameraComponent> m_cameraImpl;
};

}