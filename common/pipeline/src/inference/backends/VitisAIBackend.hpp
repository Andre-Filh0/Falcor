#pragma once

#include "IInferenceBackend.hpp"
#include <string>
#include <memory>

#ifdef FALCOR_HAVE_VITIS_AI
#include <vart/runner.hpp>
#include <vart/tensor_buffer.hpp>
#include <xir/graph/graph.hpp>
#include <xir/subgraph/subgraph.hpp>
#endif

/**
 * @brief Backend de inferencia usando o DPU Vitis AI no Kria KR260.
 *
 * Quando FALCOR_USE_VITIS_AI=ON (build para a Kria):
 *   - load()   : desserializa o .xmodel, localiza o subgrafo DPU e cria o Runner.
 *   - run()    : aloca TensorBuffers, executa inferencia assincrona no DPU e retorna
 *                o tensor de saida como std::vector<float>.
 *   - unload() : destroi o Runner e o grafo.
 *
 * Quando FALCOR_USE_VITIS_AI=OFF (build no PC / CI):
 *   - load()   : loga aviso, nao faz nada.
 *   - run()    : retorna {0.0f} (passthrough para desenvolvimento).
 *   - unload() : noop.
 *
 * Para adicionar um novo backend (ex: ONNX Runtime):
 *   1. Crie common/pipeline/src/inference/backends/ONNXBackend.hpp/.cpp
 *   2. Implemente IInferenceBackend
 *   3. Adicione a opcao no common/pipeline/CMakeLists.txt
 *   4. Em src/main.cpp troque std::make_unique<VitisAIBackend>()
 *      por     std::make_unique<ONNXBackend>()
 */
class VitisAIBackend : public IInferenceBackend {
public:
    VitisAIBackend()  = default;
    ~VitisAIBackend() override { unload(); }

    std::string_view backendName() const override { return "VitisAI-DPU"; }

    void load  (const std::string& modelPath) override;
    void unload()                             override;

    std::vector<float> run(const std::vector<float>& input) override;

private:
#ifdef FALCOR_HAVE_VITIS_AI
    std::unique_ptr<xir::Graph> m_graph;
    vart::Runner*               m_runner{nullptr};
#endif
};
