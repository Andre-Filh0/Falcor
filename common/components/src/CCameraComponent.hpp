#pragma once
#include <stdexcept> // Para std::runtime_error
#include "ComponentSystem.hpp"
#include "Logger.hpp"
// Se a pasta 'interfaces' estiver no mesmo nível da 'src'
#include "interfaces/ICameraComponent.hpp"

namespace Falcor::Components {

class CCameraComponent : public Falcor::Components::CComponent<CCameraComponent> {
public:
    enum class CameraType {
        DMV_CAMERA,    
        USB_GENERIC_CAMERA,
        MOCK_CAMERA        
    };

    // Construtor padrão (exigido pelo ComponentRegistry atual)
    CCameraComponent();
    virtual ~CCameraComponent(); 

    // Método para configurar o componente ANTES do initialize
    void configure(CameraType type, CameraConfig config);

    void initialize() override; 
    void shutdown() override;
    void getVideoBuffer();

private:
    CameraType m_selectedType = CameraType::MOCK_CAMERA;
    CameraConfig m_cameraConfig;
    std::unique_ptr<ICameraComponent> m_cameraImpl; // Ponteiro para a interface
};

}