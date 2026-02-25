#pragma once

#include "ComponentSystem.hpp"
#include "vendors/ImplCameraComponent_DMV_SDK.hpp"

namespace Falcor::Components {

class CCameraComponent : public Falcor::Components::CComponent<CCameraComponent> {
public:
    CCameraComponent();  
    virtual ~CCameraComponent(); // IMPORTANTE: Declarar destruidor virtual
    
    void initialize() override; 
    void shutdown() override;

    void getVideoBuffer();
};
}