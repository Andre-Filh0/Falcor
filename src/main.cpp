#include "ComponentSystem.hpp"
#include "CCameraComponent.hpp"
#include "CPipelineComponent.hpp"
#include "steps/PassthroughStep.hpp"
// Abstracoes de inferencia (base generica)
#include "inference/InferenceStep.hpp"
#include "inference/backends/VitisAIBackend.hpp"
#include "Logger.hpp"

using namespace Falcor::Components;

int main()
{
    // ── Configuracao da camera ────────────────────────────────────────────────
    // Parametros especificos do projeto: camera Delta GigE Vision na Kria KR260.
    // IP e interface configurados via scripts/env_setup_network.sh.
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
        Falcor::Logger.log_info("=== Falcor Vision System Iniciando ===");

        // ── Pipeline ─────────────────────────────────────────────────────────
        // Criada antes da camera: garante que a thread de processamento esta
        // pronta antes de qualquer frame chegar.
        auto* pipe = CreateComponent<CPipelineComponent>();

        // Step 1: passthrough de debug (log de cada frame)
        pipe->addStep(std::make_unique<PassthroughStep>());

        // Step 2: inferencia de peso via Vitis AI DPU.
        //
        // O InferenceStep e generico (base reutilizavel):
        //   - preprocessa o frame (bilinear resize + normalizacao)
        //   - delega a inferencia ao backend (VitisAIBackend aqui)
        //   - chama o lambda de pos-processamento para escrever o resultado
        //
        // O lambda abaixo e especifico do projeto Falcor:
        //   saida[0] e o peso normalizado [0,1] → escala para kg.
        //   Ajuste a escala conforme o treinamento do modelo.
        pipe->addStep(std::make_unique<InferenceStep>(
            std::make_unique<VitisAIBackend>(),
            "cattle_weight.xmodel",
            /*inputWidth=*/  224,
            /*inputHeight=*/ 224,
            [](const std::vector<float>& output, FrameMetadata& meta) {
                // Desnormalizacao: saida normalizada [0, 1] → peso em kg.
                // Intervalo: 0–1000 kg (adequado para Nelore adulto).
                // TODO: ajustar fator de escala conforme modelo treinado.
                meta.weight_kg = output[0] * 1000.0f;
            }
        ));

        // Handler chamado com o frame final (apos todos os steps).
        // Aqui voce pode: salvar em disco, publicar via ZMQ, atualizar UI, etc.
        pipe->setOutputHandler([](RawFrame frame) {
            Falcor::Logger.log_info(
                "==> Frame #{} processado. Peso estimado: {:.2f} kg.",
                frame.metadata.frameId,
                frame.metadata.weight_kg
            );
        });

        // ── Camera ───────────────────────────────────────────────────────────
        // Criada apos configurar a pipeline: ao conectar, pode comecar a enviar
        // frames imediatamente sem perda.
        auto* cam = CreateComponent<CCameraComponent>(
            CCameraComponent::CameraType::DMV_CAMERA,
            config
        );

        // ── Loop de captura ──────────────────────────────────────────────────
        // getVideoBuffer() bloqueia ate um frame estar disponivel.
        // pushFrame()      deposita no DoubleBuffer da pipeline (backpressure
        //                  se a pipeline nao acompanhar o FPS da camera).
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

    // Encerra os componentes em ordem REVERSA de criacao:
    //   Camera (para de enviar frames) → Pipeline (drena buffer, encerra thread).
    // A ordem e garantida automaticamente pelo ComponentRegistry.
    ComponentRegistry::shutdown_all();

    return EXIT_SUCCESS;
}
