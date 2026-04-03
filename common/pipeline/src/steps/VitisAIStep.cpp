#include "VitisAIStep.hpp"
#include "Logger.hpp"

VitisAIStep::VitisAIStep(std::string xmodelPath, uint32_t inputWidth, uint32_t inputHeight)
    : m_modelPath(std::move(xmodelPath))
    , m_inputWidth(inputWidth)
    , m_inputHeight(inputHeight)
{}

VitisAIStep::~VitisAIStep()
{
    shutdown();
}

void VitisAIStep::initialize()
{
#ifdef FALCOR_HAVE_VITIS_AI
    Falcor::Logger.log_info("[VitisAI] Carregando modelo: '{}'.", m_modelPath);

    m_graph = xir::Graph::deserialize(m_modelPath);

    // O subgrafo do DPU e o unico filho da raiz com device="DPU"
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
        throw std::runtime_error("[VitisAI] Nenhum subgrafo DPU encontrado no modelo.");

    m_runner = vart::Runner::create_runner(dpu_subgraph, "run").release();

    Falcor::Logger.log_info("[VitisAI] DPU Runner criado. Input: {}x{}.",
        m_inputWidth, m_inputHeight);

#else
    Falcor::Logger.log_warning(
        "[VitisAI] Vitis AI nao compilado (FALCOR_USE_VITIS_AI=OFF). "
        "VitisAIStep em modo passthrough.");
#endif
}

void VitisAIStep::shutdown()
{
#ifdef FALCOR_HAVE_VITIS_AI
    if (m_runner)
    {
        delete m_runner;
        m_runner = nullptr;
    }
    m_graph.reset();
    Falcor::Logger.log_info("[VitisAI] Runner encerrado.");
#endif
}

RawFrame VitisAIStep::process(RawFrame frame)
{
#ifdef FALCOR_HAVE_VITIS_AI
    if (!m_runner)
    {
        Falcor::Logger.log_error("[VitisAI] Runner nao inicializado.");
        return frame;
    }

    // Aloca buffers de entrada e saida
    auto inputTensors  = m_runner->get_input_tensors();
    auto outputTensors = m_runner->get_output_tensors();

    auto inputBuf  = vart::alloc_cpu_flat_tensor_buffer(inputTensors);
    auto outputBuf = vart::alloc_cpu_flat_tensor_buffer(outputTensors);

    // Preprocessamento: RGB8 RawFrame → float32 normalizado
    preprocessFrame(frame, inputBuf.get());

    // Executa inferencia no DPU (assincrono + wait)
    auto job = m_runner->execute_async(
        {inputBuf.get()}, {outputBuf.get()});
    m_runner->wait(job.first, -1);

    // Postprocessamento: tensor de saida → peso em kg
    frame.metadata.weight_kg = postprocessOutput(outputBuf.get());

    Falcor::Logger.log_info("[VitisAI] Frame #{} — peso estimado: {:.2f} kg.",
        frame.metadata.frameId, frame.metadata.weight_kg);

#else
    // Passthrough no PC sem Vitis AI
    (void)m_inputWidth;
    (void)m_inputHeight;
#endif

    return frame;
}

#ifdef FALCOR_HAVE_VITIS_AI

void VitisAIStep::preprocessFrame(const RawFrame& frame,
                                   vart::TensorBuffer* inputBuf) const
{
    // Obtem ponteiro para o buffer de entrada do DPU
    auto [data, size] = inputBuf->data();
    auto* dst = reinterpret_cast<float*>(data);

    const uint32_t srcW = frame.width;
    const uint32_t srcH = frame.height;
    const uint8_t* src  = frame.ptr();

    // Redimensionamento simples por nearest-neighbor para m_inputWidth x m_inputHeight.
    // TODO: substituir por redimensionamento bilinear para melhor qualidade.
    for (uint32_t y = 0; y < m_inputHeight; ++y)
    {
        for (uint32_t x = 0; x < m_inputWidth; ++x)
        {
            const uint32_t srcX = x * srcW / m_inputWidth;
            const uint32_t srcY = y * srcH / m_inputHeight;
            const size_t   idx  = (srcY * srcW + srcX) * 3;

            const size_t dstIdx = (y * m_inputWidth + x) * 3;
            dst[dstIdx + 0] = static_cast<float>(src[idx + 0]) / 255.0f; // R
            dst[dstIdx + 1] = static_cast<float>(src[idx + 1]) / 255.0f; // G
            dst[dstIdx + 2] = static_cast<float>(src[idx + 2]) / 255.0f; // B
        }
    }
}

float VitisAIStep::postprocessOutput(vart::TensorBuffer* outputBuf) const
{
    // O modelo de regressao de peso produz um unico valor float.
    // TODO: ajustar de acordo com a arquitetura e escala do modelo treinado.
    auto [data, size] = outputBuf->data();
    const float raw = *reinterpret_cast<const float*>(data);

    // Desnormalizacao: assumindo saida normalizada [0,1] mapeada para [0, 1000] kg.
    // Ajustar conforme o treinamento do modelo.
    return raw * 1000.0f;
}

#endif // FALCOR_HAVE_VITIS_AI
