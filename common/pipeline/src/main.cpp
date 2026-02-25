#include "vendors/ImplCameraComponent_DMV_SDK.hpp"

int main()
{
    CameraConfig config(
        1920,
        1080,
        PixelFormat::Mono8,
        30
    );

    ImplCameraComponent_DMV_SDK camera(config);

    CameraStatus status = camera.connect(config);

    if (status != CameraStatus::Ok)
    {
        Falcor::Logger.log_error("Falha ao conectar camera.");
        return -1;
    }

    Falcor::Logger.log_info("Camera conectada com sucesso!");

    camera.disconnect();

    Falcor::Logger.log_info("Camera desconectada.");

    return 0;
}