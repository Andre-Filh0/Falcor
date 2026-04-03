#pragma once

#include <string>
#include <string_view>
#include <vector>

/**
 * @brief Interface generica para backends de inferencia de ML.
 *
 * Permite trocar o motor de inferencia (Vitis AI, ONNX Runtime, TFLite, etc.)
 * sem alterar o InferenceStep ou o restante da pipeline.
 *
 * Contrato:
 *   - load()   e chamado uma vez em InferenceStep::initialize().
 *   - run()    e chamado por frame, na thread da pipeline.
 *   - unload() e chamado uma vez em InferenceStep::shutdown().
 *
 * Exemplo de implementacoes:
 *   class VitisAIBackend  : public IInferenceBackend { ... }; // Kria KR260 DPU
 *   class ONNXBackend     : public IInferenceBackend { ... }; // CPU/GPU via ONNX RT
 *   class TFLiteBackend   : public IInferenceBackend { ... }; // embarcado ARM
 *   class MockBackend     : public IInferenceBackend { ... }; // testes offline
 */
class IInferenceBackend {
public:
    virtual ~IInferenceBackend() = default;

    /**
     * @brief Carrega o modelo do disco e prepara o hardware.
     *
     * Lanca std::runtime_error se o modelo nao for encontrado
     * ou o hardware nao estiver disponivel.
     *
     * @param modelPath  Caminho para o arquivo de modelo (ex: .xmodel, .onnx, .tflite).
     */
    virtual void load(const std::string& modelPath) = 0;

    /**
     * @brief Executa inferencia sincrona sobre o tensor de entrada.
     *
     * @param input   Tensor float32 HWC normalizado [0.0, 1.0].
     *                Tamanho = inputWidth * inputHeight * channels (RGB = 3).
     *
     * @return        Tensor de saida raw do modelo (float32).
     *                Layout e tamanho dependem da arquitetura do modelo.
     */
    virtual std::vector<float> run(const std::vector<float>& input) = 0;

    /**
     * @brief Libera todos os recursos alocados em load().
     *
     * Pode ser chamado multiplas vezes com seguranca (idempotente).
     */
    virtual void unload() = 0;

    /** Nome do backend — usado em logs e diagnostico. */
    virtual std::string_view backendName() const = 0;
};
