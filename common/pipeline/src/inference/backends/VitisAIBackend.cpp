#include "VitisAIBackend.hpp"
#include "Logger.hpp"

#include <stdexcept>

// ─────────────────────────────────────────────────────────────────────────────
// Build COM Vitis AI (Kria KR260)
// ─────────────────────────────────────────────────────────────────────────────

#ifdef FALCOR_HAVE_VITIS_AI

void VitisAIBackend::load(const std::string& modelPath)
{
    Falcor::Logger.log_info("[VitisAI] Carregando modelo: '{}'.", modelPath);

    m_graph = xir::Graph::deserialize(modelPath);
    if (!m_graph)
        throw std::runtime_error("[VitisAI] Falha ao deserializar o modelo: " + modelPath);

    // Localiza o subgrafo do DPU: unico filho da raiz com device="DPU".
    auto* root = m_graph->get_root_subgraph();
    xir::Subgraph* dpu_subgraph = nullptr;

    for (auto* child : root->get_children())
    {
        if (child->has_attr("device") &&
            child->get_attr<std::string>("device") == "DPU")
        {
            dpu_subgraph = child;
            break;
        }
    }

    if (!dpu_subgraph)
        throw std::runtime_error("[VitisAI] Nenhum subgrafo DPU encontrado em: " + modelPath);

    m_runner = vart::Runner::create_runner(dpu_subgraph, "run").release();
    if (!m_runner)
        throw std::runtime_error("[VitisAI] Falha ao criar o DPU Runner.");

    Falcor::Logger.log_info("[VitisAI] DPU Runner criado com sucesso.");
}

void VitisAIBackend::unload()
{
    if (m_runner) {
        delete m_runner;
        m_runner = nullptr;
    }
    m_graph.reset();
    Falcor::Logger.log_info("[VitisAI] Runner encerrado.");
}

std::vector<float> VitisAIBackend::run(const std::vector<float>& input)
{
    if (!m_runner)
        throw std::runtime_error("[VitisAI] run() chamado sem runner inicializado.");

    auto inputTensors  = m_runner->get_input_tensors();
    auto outputTensors = m_runner->get_output_tensors();

    auto inputBuf  = vart::alloc_cpu_flat_tensor_buffer(inputTensors);
    auto outputBuf = vart::alloc_cpu_flat_tensor_buffer(outputTensors);

    // Copia o tensor de entrada para o buffer do DPU.
    auto [inData, inSize] = inputBuf->data();
    auto* dst = reinterpret_cast<float*>(inData);
    std::copy(input.begin(), input.end(), dst);

    // Executa inferencia assincrona e aguarda conclusao.
    auto job = m_runner->execute_async({inputBuf.get()}, {outputBuf.get()});
    m_runner->wait(job.first, /*timeout_ms=*/-1);

    // Copia o tensor de saida para std::vector<float>.
    auto [outData, outSize] = outputBuf->data();
    const float* src = reinterpret_cast<const float*>(outData);
    const size_t count = outSize / sizeof(float);

    return std::vector<float>(src, src + count);
}

// ─────────────────────────────────────────────────────────────────────────────
// Build SEM Vitis AI (PC / CI / testes offline)
// ─────────────────────────────────────────────────────────────────────────────

#else // FALCOR_HAVE_VITIS_AI

void VitisAIBackend::load(const std::string& modelPath)
{
    Falcor::Logger.log_warning(
        "[VitisAI] FALCOR_USE_VITIS_AI=OFF — backend em modo passthrough. "
        "Modelo '{}' nao sera carregado.", modelPath);
}

void VitisAIBackend::unload()
{
    // Noop no modo passthrough.
}

std::vector<float> VitisAIBackend::run(const std::vector<float>& /*input*/)
{
    // Retorna um unico zero — o pos-processamento em main.cpp recebera 0.0f.
    return {0.0f};
}

#endif // FALCOR_HAVE_VITIS_AI
