// CCameraComponent.cpp
#include "CCameraComponent.hpp"

Falcor::Components::CCameraComponent::CCameraComponent() : CComponent() {
    
}
Falcor::Components::CCameraComponent::~CCameraComponent() {}

void Falcor::Components::CCameraComponent::initialize() {
    std::cout << "Component Camera Initialized" << std::endl;
}

void Falcor::Components::CCameraComponent::shutdown() {
    std::cout << "Component Camera Destroyed" << std::endl;
}

void Falcor::Components::CCameraComponent::getVideoBuffer()
{
    std::cout << "Showing video" << std::endl;
}
