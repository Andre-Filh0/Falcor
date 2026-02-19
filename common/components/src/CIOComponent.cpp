// CIOComponent.cpp
#include "CIOComponent.hpp"

Falcor::Components::CIOComponent::CIOComponent() : CComponent() {}
Falcor::Components::CIOComponent::~CIOComponent() {}

void Falcor::Components::CIOComponent::initialize() {
    std::cout << "Component IO Initialized" << std::endl;
}

void Falcor::Components::CIOComponent::shutdown() {
    std::cout << "Component IO Destroyed" << std::endl;
}