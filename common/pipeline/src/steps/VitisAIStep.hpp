#pragma once

#include "IPipelineStep.hpp"
#include <string>
#include <memory>

#ifdef FALCOR_HAVE_VITIS_AI
// VART (Vitis AI Runtime)
#include <vart/runner.hpp>
#include <vart/tensor_buffer.hpp>
#include <xir/graph/graph.hpp>
#include <xir/subgraph/subgraph.hpp>
#endif

/**
 * @brief Step de inferencia usando o DPU Vitis AI no Kria KR260.
 *
 * Fluxo quando FALCOR_HAVE_VITIS_AI=ON:
 *   1. Preprocessamento: RawFrame RGB8 → tensor float32 normalizado (HWC)
 *   2. Inferencia no DPU via VART Runner
 *   3. Postprocessamento: saida do modelo → peso em kg
 *   4. Resultado armazenado em frame.metadata.weight_kg
 *
 * Quando FALCOR_HAVE_VITIS_AI=OFF (build no PC):
 *   - Passthrough com log de aviso. weight_kg permanece 0.0f.
 *
 * @param xmodelPath  Caminho para o arquivo .xmodel compilado para o DPU.
 * @param inputWidth  Largura esperada pelo modelo (ex: 224).
 * @param inputHeight Altura esperada pelo modelo (ex: 224).
 */
class VitisAIStep : public IPipelineStep {
public:
    VitisAIStep(std::string xmodelPath, uint32_t inputWidth, uint32_t inputHeight);
    ~VitisAIStep() override;

    std::string_view name() const override { return "VitisAI"; }

    void initialize() override;
    void shutdown()   override;

    RawFrame process(RawFrame frame) override;

private:
    std::string m_modelPath;
    uint32_t    m_inputWidth;
    uint32_t    m_inputHeight;

#ifdef FALCOR_HAVE_VITIS_AI
    std::unique_ptr<xir::Graph>   m_graph;
    vart::Runner*                 m_runner{nullptr};

    // Converte RawFrame RGB8 para o tensor de entrada do modelo.
    // Redimensiona para (m_inputWidth x m_inputHeight) e normaliza [0,255] -> [0.0,1.0].
    void preprocessFrame(const RawFrame& frame,
                         vart::TensorBuffer* inputBuf) const;

    // Le a saida do DPU e converte para peso em kg.
    float postprocessOutput(vart::TensorBuffer* outputBuf) const;
#endif
};
