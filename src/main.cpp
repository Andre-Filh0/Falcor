#include "ComponentSystem.hpp"
#include "CCameraComponent.hpp"
#include "Logger.hpp"

using namespace Falcor::Components;

int main() {
    // ---------------------------------------------------------
    // 1. CONFIGURAÇÃO INDUSTRIAL (C++20)
    // ---------------------------------------------------------
    // Alinhado com a estrutura GigEVisionProtocol -> CameraProtocol -> CameraConfig
    CameraConfig config = {
        .protocol = {
            .GigE = {
                .camera_ip_str = "192.168.137.100",
                .interface = "eth1"
            }
        },
        .width = 1920,
        .height = 1080,
        .format = PixelFormat::RGB8,
        .fps = 30
    };

    try {
        Falcor::Logger.log_info("=== Falcor Vision System Booting (Kria KV260) ===");

        // ---------------------------------------------------------
        // 2. REGISTRO E INICIALIZAÇÃO AUTOMÁTICA
        // ---------------------------------------------------------
        // O ComponentRegistry recebe os argumentos, chama configure(type, config)
        // e logo em seguida executa initialize() para conectar o hardware.
        auto* camera = GetOrCreateComponent<CCameraComponent>(
            CCameraComponent::CameraType::DMV_CAMERA, 
            config
        );

        // ---------------------------------------------------------
        // 3. LOOP DE EXECUÇÃO
        // ---------------------------------------------------------
        if (camera) {
            for(int i = 0; i < 5; ++i) {
                camera->getVideoBuffer();
            }
        }

        Falcor::Logger.log_info("Processamento de teste concluido.");

    } catch (const std::exception& e) {
        // Captura falhas de IP, driver ou alocação
        Falcor::Logger.log_error("FALHA FATAL NO SISTEMA: {}", e.what());
        return EXIT_FAILURE;
    }

    // ---------------------------------------------------------
    // 4. SHUTDOWN SEGURO
    // ---------------------------------------------------------
    // Desconecta drivers e limpa a memória de todos os singletons
    ComponentRegistry::shutdown_all();
    
    return EXIT_SUCCESS;
}