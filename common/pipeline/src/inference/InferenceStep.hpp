#pragma once

#include "IPipelineStep.hpp"
#include "IInferenceBackend.hpp"
#include <functional>
#include <memory>
#include <string>

/**
 * @brief Step de inferencia generica para a pipeline de processamento.
 *
 * Encapsula o ciclo completo de inferencia:
 *   1. Preprocessamento: RawFrame RGB8 → tensor float32 HWC normalizado [0,1]
 *      usando interpolacao bilinear para redimensionamento de alta qualidade.
 *   2. Inferencia: delegada ao IInferenceBackend configurado
 *      (Vitis AI DPU, ONNX Runtime, TFLite, mock, etc.).
 *   3. Pos-processamento: tensor de saida → escrita em FrameMetadata,
 *      via funcao de callback definida pelo chamador (logica do dominio).
 *
 * Design:
 *   - O STEP e generico e reutilizavel (pertence a base, common/).
 *   - O BACKEND e intercambivel — basta implementar IInferenceBackend.
 *   - O POS-PROCESSAMENTO fica no codigo do projeto (src/), via PostprocessFn.
 *
 * Uso tipico (em src/main.cpp):
 * @code
 *   pipe->addStep(std::make_unique<InferenceStep>(
 *       std::make_unique<VitisAIBackend>(),
 *       "cattle_weight.xmodel",
 *       224, 224,
 *       [](const std::vector<float>& out, FrameMetadata& meta) {
 *           meta.weight_kg = out[0] * 1000.0f;
 *       }
 *   ));
 * @endcode
 *
 * @param backend       Implementacao do motor de inferencia.
 * @param modelPath     Caminho para o arquivo de modelo.
 * @param inputWidth    Largura esperada pelo modelo (pixels).
 * @param inputHeight   Altura esperada pelo modelo (pixels).
 * @param postprocess   Funcao que converte a saida do modelo em metadados do frame.
 */
class InferenceStep : public IPipelineStep {
public:
    /**
     * @brief Funcao de pos-processamento injetada pelo codigo do projeto.
     *
     * Recebe o tensor de saida raw do modelo e preenche os campos
     * relevantes do FrameMetadata (ex: weight_kg, class_id, confidence).
     */
    using PostprocessFn = std::function<void(const std::vector<float>&, FrameMetadata&)>;

    InferenceStep(std::unique_ptr<IInferenceBackend> backend,
                  std::string                        modelPath,
                  uint32_t                           inputWidth,
                  uint32_t                           inputHeight,
                  PostprocessFn                      postprocess);

    ~InferenceStep() override;

    std::string_view name() const override;

    void     initialize() override;
    void     shutdown()   override;
    RawFrame process(RawFrame frame) override;

private:
    /**
     * @brief Redimensiona o frame para (inputWidth x inputHeight) com interpolacao
     * bilinear e normaliza os valores de pixel de [0, 255] para [0.0, 1.0].
     *
     * Saida: vetor float32 HWC com tamanho inputWidth * inputHeight * 3.
     */
    std::vector<float> preprocessFrame(const RawFrame& frame) const;

    std::unique_ptr<IInferenceBackend> m_backend;
    std::string                        m_modelPath;
    uint32_t                           m_inputWidth;
    uint32_t                           m_inputHeight;
    PostprocessFn                      m_postprocess;
};
