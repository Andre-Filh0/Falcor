#include "InferenceStep.hpp"
#include "Logger.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

// ─────────────────────────────────────────────────────────────────────────────
// Construtor / Destrutor
// ─────────────────────────────────────────────────────────────────────────────

InferenceStep::InferenceStep(std::unique_ptr<IInferenceBackend> backend,
                             std::string                        modelPath,
                             uint32_t                           inputWidth,
                             uint32_t                           inputHeight,
                             PostprocessFn                      postprocess)
    : m_backend     (std::move(backend))
    , m_modelPath   (std::move(modelPath))
    , m_inputWidth  (inputWidth)
    , m_inputHeight (inputHeight)
    , m_postprocess (std::move(postprocess))
{
    if (!m_backend)
        throw std::invalid_argument("[InferenceStep] backend nao pode ser nullptr.");
    if (!m_postprocess)
        throw std::invalid_argument("[InferenceStep] postprocess nao pode ser nullptr.");
}

InferenceStep::~InferenceStep()
{
    shutdown();
}

std::string_view InferenceStep::name() const
{
    return m_backend->backendName();
}

// ─────────────────────────────────────────────────────────────────────────────
// Ciclo de vida
// ─────────────────────────────────────────────────────────────────────────────

void InferenceStep::initialize()
{
    Falcor::Logger.log_info("[InferenceStep] Carregando modelo '{}' via backend '{}'.",
        m_modelPath, m_backend->backendName());

    m_backend->load(m_modelPath);

    Falcor::Logger.log_info("[InferenceStep] Backend pronto. Input: {}x{}.",
        m_inputWidth, m_inputHeight);
}

void InferenceStep::shutdown()
{
    if (m_backend) {
        m_backend->unload();
        Falcor::Logger.log_info("[InferenceStep] Backend '{}' encerrado.",
            m_backend->backendName());
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Processamento de frame
// ─────────────────────────────────────────────────────────────────────────────

RawFrame InferenceStep::process(RawFrame frame)
{
    // 1. Preprocessamento: RawFrame → tensor float32 HWC normalizado
    auto inputTensor = preprocessFrame(frame);

    // 2. Inferencia delegada ao backend
    auto outputTensor = m_backend->run(inputTensor);

    // 3. Pos-processamento: tensor de saida → metadados do frame
    //    (logica do dominio injetada pelo codigo do projeto em src/)
    m_postprocess(outputTensor, frame.metadata);

    Falcor::Logger.log_info("[InferenceStep] Frame #{} processado.",
        frame.metadata.frameId);

    return frame;
}

// ─────────────────────────────────────────────────────────────────────────────
// Pre-processamento: bilinear resize + normalizacao
// ─────────────────────────────────────────────────────────────────────────────

std::vector<float> InferenceStep::preprocessFrame(const RawFrame& frame) const
{
    const uint32_t srcW = frame.width;
    const uint32_t srcH = frame.height;
    const uint8_t* src  = frame.ptr();

    const size_t outputSize = m_inputWidth * m_inputHeight * 3;
    std::vector<float> dst(outputSize);

    // Interpolacao bilinear com offset de meio pixel para alinhar centros.
    // Formula: srcCoord = (dstCoord + 0.5) * (srcDim / dstDim) - 0.5
    // Isso evita os artefatos de borda que o nearest-neighbor produz.
    for (uint32_t y = 0; y < m_inputHeight; ++y)
    {
        const float fSrcY = (y + 0.5f) * static_cast<float>(srcH) / m_inputHeight - 0.5f;
        const int   y0    = static_cast<int>(std::floor(fSrcY));
        const int   y1    = y0 + 1;
        const float wy    = fSrcY - y0;

        // Clamp para nao sair dos limites da imagem fonte.
        const uint32_t cy0 = static_cast<uint32_t>(std::clamp(y0, 0, static_cast<int>(srcH - 1)));
        const uint32_t cy1 = static_cast<uint32_t>(std::clamp(y1, 0, static_cast<int>(srcH - 1)));

        for (uint32_t x = 0; x < m_inputWidth; ++x)
        {
            const float fSrcX = (x + 0.5f) * static_cast<float>(srcW) / m_inputWidth - 0.5f;
            const int   x0    = static_cast<int>(std::floor(fSrcX));
            const int   x1    = x0 + 1;
            const float wx    = fSrcX - x0;

            const uint32_t cx0 = static_cast<uint32_t>(std::clamp(x0, 0, static_cast<int>(srcW - 1)));
            const uint32_t cx1 = static_cast<uint32_t>(std::clamp(x1, 0, static_cast<int>(srcW - 1)));

            const size_t dstIdx = (y * m_inputWidth + x) * 3;

            for (uint32_t c = 0; c < 3; ++c)
            {
                // Quatro vizinhos para interpolacao bilinear (2x2)
                const float v00 = src[(cy0 * srcW + cx0) * 3 + c];
                const float v01 = src[(cy0 * srcW + cx1) * 3 + c];
                const float v10 = src[(cy1 * srcW + cx0) * 3 + c];
                const float v11 = src[(cy1 * srcW + cx1) * 3 + c];

                // Pesos: interpolacao linear em x e depois em y
                const float v = v00 * (1.0f - wx) * (1.0f - wy)
                              + v01 *          wx  * (1.0f - wy)
                              + v10 * (1.0f - wx)  *          wy
                              + v11 *          wx  *          wy;

                // Normaliza [0, 255] → [0.0, 1.0]
                dst[dstIdx + c] = v / 255.0f;
            }
        }
    }

    return dst;
}
