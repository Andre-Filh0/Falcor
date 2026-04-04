#include "Camera.hpp"
#include "Logger.hpp"

// ─────────────────────────────────────────────────────────────────────────────
// Helpers internos
// ─────────────────────────────────────────────────────────────────────────────

static ImplCameraComponent_DMV_SDK* getDelta(CCameraComponent& cam)
{
    return cam.getDeltaImpl();
}

static const ImplCameraComponent_DMV_SDK* getDeltaConst(const CCameraComponent& cam)
{
    return cam.getDeltaImpl();
}

// ─────────────────────────────────────────────────────────────────────────────
namespace Falcor::Delta {
// ─────────────────────────────────────────────────────────────────────────────

// ── Descoberta de câmeras ─────────────────────────────────────────────────────

std::vector<DeltaDiscoveredCamera> enumerateCameras(uint32_t timeoutMs)
{
    Falcor::Logger.log_info("[Delta] Varrendo rede em busca de câmeras Delta GigE Vision...");
    auto cameras = ImplCameraComponent_DMV_SDK::enumerateCameras(timeoutMs);

    if (cameras.empty())
    {
        Falcor::Logger.log_warning("[Delta] Nenhuma câmera encontrada. "
            "Verifique cabos, switch e endereçamento IP da interface de rede.");
    }
    else
    {
        Falcor::Logger.log_info("[Delta] ── Câmeras encontradas ({}) ─────────────────", cameras.size());
        for (size_t i = 0; i < cameras.size(); ++i)
        {
            const auto& c = cameras[i];
            Falcor::Logger.log_info("[Delta]   [{}] Modelo={:<20} Serial={:<16} IP={:<16} MAC={}",
                i, c.model, c.serial, c.ip, c.mac);
        }
        Falcor::Logger.log_info("[Delta] ─────────────────────────────────────────────");
    }

    return cameras;
}

// ── User Presets ──────────────────────────────────────────────────────────────

CameraStatus loadPreset(CCameraComponent& cam, DeltaUserPreset preset)
{
    auto* impl = getDelta(cam);
    if (!impl)
    {
        Falcor::Logger.log_error("[Delta] loadPreset: câmera não usa backend Delta DMV-SDK.");
        return CameraStatus::InternalError;
    }
    return impl->loadUserPreset(preset);
}

CameraStatus savePreset(CCameraComponent& cam, DeltaUserPreset preset)
{
    auto* impl = getDelta(cam);
    if (!impl)
    {
        Falcor::Logger.log_error("[Delta] savePreset: câmera não usa backend Delta DMV-SDK.");
        return CameraStatus::InternalError;
    }
    return impl->saveUserPreset(preset);
}

CameraStatus setBootPreset(CCameraComponent& cam, DeltaUserPreset preset)
{
    auto* impl = getDelta(cam);
    if (!impl)
    {
        Falcor::Logger.log_error("[Delta] setBootPreset: câmera não usa backend Delta DMV-SDK.");
        return CameraStatus::InternalError;
    }
    return impl->setDefaultPreset(preset);
}

// ── Informações da câmera ─────────────────────────────────────────────────────

DeltaCameraInfo getCameraInfo(const CCameraComponent& cam)
{
    const auto* impl = getDeltaConst(cam);
    if (!impl)
    {
        Falcor::Logger.log_error("[Delta] getCameraInfo: câmera não usa backend Delta DMV-SDK.");
        return {};
    }
    return impl->queryCameraInfo();
}

void logCameraInfo(const CCameraComponent& cam)
{
    const auto* impl = getDeltaConst(cam);
    if (!impl)
    {
        Falcor::Logger.log_error("[Delta] logCameraInfo: câmera não usa backend Delta DMV-SDK.");
        return;
    }
    impl->logCameraInfo();
}

// ── Feature Stream ────────────────────────────────────────────────────────────

CameraStatus saveFeatureStream(const CCameraComponent& cam, const std::string& filepath)
{
    const auto* impl = getDeltaConst(cam);
    if (!impl)
    {
        Falcor::Logger.log_error("[Delta] saveFeatureStream: câmera não usa backend Delta DMV-SDK.");
        return CameraStatus::InternalError;
    }
    return impl->saveFeatureStreamToFile(filepath);
}

CameraStatus loadFeatureStream(CCameraComponent& cam, const std::string& filepath)
{
    auto* impl = getDelta(cam);
    if (!impl)
    {
        Falcor::Logger.log_error("[Delta] loadFeatureStream: câmera não usa backend Delta DMV-SDK.");
        return CameraStatus::InternalError;
    }
    return impl->loadFeatureStreamFromFile(filepath);
}

// ── Utilitários ───────────────────────────────────────────────────────────────

CameraConfig makeConfig(const std::string& ip,
                        uint32_t width,
                        uint32_t height,
                        double   fps)
{
    CameraConfig cfg;
    cfg.protocol.GigE.camera_ip = ip;
    cfg.width  = width;
    cfg.height = height;
    cfg.fps    = fps;
    cfg.format = PixelFormat::RGB8; // padrão do projeto Falcor
    return cfg;
}

} // namespace Falcor::Delta
