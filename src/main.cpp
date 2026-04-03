#include "ComponentSystem.hpp"
#include "CCameraComponent.hpp"
#include "Logger.hpp"

using namespace Falcor::Components;

int main()
{
    CameraConfig config = {
        .protocol = {
            .GigE = {
                .camera_ip = "192.168.137.100",
                .interface_name = "eth1"
            }
        },
        .width = 1440,
        .height = 1080,
        .format = PixelFormat::RGB8,
        .fps = 60
    };

    try
    {
        Falcor::Logger.log_info("=== Falcor Vision System Booting ===");

        auto* camera = GetOrCreateComponent<CCameraComponent>(
            CCameraComponent::CameraType::DMV_CAMERA,
            config
        );
        
        if (camera)
        {
            for (int i = 0; i < 5; ++i)
            {
                camera->getVideoBuffer();
            }
        }

        Falcor::Logger.log_info("Teste de captura finalizado.");
    }
    catch (const std::exception& e)
    {
        Falcor::Logger.log_error("FALHA FATAL: {}", e.what());
        return EXIT_FAILURE;
    }

    ComponentRegistry::shutdown_all();
    return EXIT_SUCCESS;
}