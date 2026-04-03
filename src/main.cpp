#include "ComponentSystem.hpp"
#include "CCameraComponent.hpp"
#include "CPipelineComponent.hpp"
#include "steps/PassthroughStep.hpp"
#include "steps/VitisAIStep.hpp"
#include "Logger.hpp"

using namespace Falcor::Components;

int main()
{
    CameraConfig config = {
        .protocol = {
            .GigE = {
                .camera_ip      = "192.168.137.100",
                .interface_name = "eth1"
            }
        },
        .width  = 1440,
        .height = 1080,
        .format = PixelFormat::RGB8,
        .fps    = 60
    };

    try
    {
        Falcor::Logger.log_info("=== Falcor Vision System Booting ===");

        // ── Pipeline ─────────────────────────────────────────────
        // Criado antes da camera para garantir que esta pronto ao
        // receber o primeiro frame.

        auto* pipe = CreateComponent<CPipelineComponent>();

        pipe->addStep(std::make_unique<PassthroughStep>());

        // Na Kria com FALCOR_USE_VITIS_AI=ON: inferencia real no DPU.
        // No PC sem Vitis AI: passthrough com log de aviso.
        pipe->addStep(std::make_unique<VitisAIStep>("cattle_weight.xmodel", 224, 224));

        // Handler chamado com o frame final (apos todos os steps).
        // Aqui voce pode: salvar em disco, publicar via ZMQ, atualizar UI, etc.
        pipe->setOutputHandler([](RawFrame frame) {
            Falcor::Logger.log_info(
                "==> Frame #{} processado. Peso estimado: {:.2f} kg.",
                frame.metadata.frameId,
                frame.metadata.weight_kg
            );
        });

        // initialize() e chamado automaticamente pelo CreateComponent:
        // lanca a thread de processamento da pipeline.

        // ── Camera ───────────────────────────────────────────────
        // Criada apos a pipeline — ao conectar ja pode comecar a enviar frames.
        auto* cam = CreateComponent<CCameraComponent>(
            CCameraComponent::CameraType::DMV_CAMERA,
            config
        );

        // ── Loop de captura ──────────────────────────────────────
        // getVideoBuffer() captura um frame da camera.
        // pushFrame() deposita no double buffer da pipeline (pode bloquear
        // se a pipeline nao acompanhar — backpressure intencional).
        constexpr int kFramesToCapture = 30;

        for (int i = 0; i < kFramesToCapture; ++i)
        {
            auto frame = cam->getVideoBuffer();

            if (frame)
                pipe->pushFrame(std::move(*frame));
        }

        Falcor::Logger.log_info("Captura finalizada. Aguardando pipeline processar...");
    }
    catch (const std::exception& e)
    {
        Falcor::Logger.log_error("FALHA FATAL: {}", e.what());
        return EXIT_FAILURE;
    }

    // Encerra todos os componentes na ordem inversa de criacao:
    // camera primeiro (para stream), depois pipeline (drena o buffer e encerra thread).
    ComponentRegistry::shutdown_all();

    return EXIT_SUCCESS;
}
