#pragma once

#include "ComponentSystem.hpp"

namespace Falcor::Components {

class CIOComponent : public Falcor::Components::CComponent<CIOComponent> {
public:
    CIOComponent();  
    virtual ~CIOComponent(); // IMPORTANTE: Declarar destruidor virtual
    
    void initialize() override; 
    void shutdown() override;
};
}