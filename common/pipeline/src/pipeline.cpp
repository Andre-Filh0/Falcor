#include "Pipeline.hpp"
#include "Logger.hpp"

// ─────────────────────────────────────────────────────────────────────────────
// Configuração da pipeline
// ─────────────────────────────────────────────────────────────────────────────

void Pipeline::addStep(std::unique_ptr<IPipelineStep> step)
{
    // Inicializa o step antes de inserir — pode lançar se hardware não disponível.
    step->initialize();

    std::lock_guard<std::mutex> lock(m_configMutex);
    Falcor::Logger.log_info("[Pipeline] Step adicionado: '{}'.", step->name());
    m_steps.push_back(std::move(step));
}

void Pipeline::setOutputHandler(FrameCallback handler)
{
    std::lock_guard<std::mutex> lock(m_configMutex);
    m_outputHandler = std::move(handler);
}

// ─────────────────────────────────────────────────────────────────────────────
// Ciclo de vida da thread
// ─────────────────────────────────────────────────────────────────────────────

void Pipeline::start()
{
    if (m_running.exchange(true)) {
        Falcor::Logger.log_warning("[Pipeline] start() chamado mas pipeline ja esta rodando.");
        return;
    }

    m_thread = std::thread(&Pipeline::processingLoop, this);
    Falcor::Logger.log_info("[Pipeline] Thread de processamento iniciada.");
}

void Pipeline::stop()
{
    if (!m_running.exchange(false))
        return; // ja parada ou nunca iniciada

    // Acorda o consumidor imediatamente, sem esperar o timeout expirar.
    m_buffer.wakeup();

    if (m_thread.joinable())
        m_thread.join();

    // Encerra cada step apos a thread terminar — sem race condition.
    {
        std::lock_guard<std::mutex> lock(m_configMutex);
        for (auto& step : m_steps)
            step->shutdown();
    }

    Falcor::Logger.log_info("[Pipeline] Thread encerrada. {} step(s) finalizados.", m_steps.size());
}

void Pipeline::pushFrame(RawFrame frame)
{
    m_buffer.produce(std::move(frame));
}

// ─────────────────────────────────────────────────────────────────────────────
// Loop de processamento (roda na thread dedicada)
// ─────────────────────────────────────────────────────────────────────────────

void Pipeline::processingLoop()
{
    Falcor::Logger.log_info("[Pipeline] Aguardando frames...");

    while (m_running.load(std::memory_order_relaxed))
    {
        RawFrame frame;

        // Aguarda um frame por ate 100 ms.
        // Timeout normal: volta a verificar m_running.
        // wakeup() encurta a espera no encerramento.
        if (!m_buffer.consume(frame, std::chrono::milliseconds(100)))
            continue;

        // Encadeia os steps em sequencia.
        // O lock e liberado antes do outputHandler para evitar
        // contencao caso o handler seja lento (ex: salvar em disco).
        {
            std::lock_guard<std::mutex> lock(m_configMutex);
            for (auto& step : m_steps)
                frame = step->process(std::move(frame));
        }

        if (m_outputHandler)
            m_outputHandler(std::move(frame));
    }

    // ── Drena frames que ficaram no buffer apos o sinal de parada ────────────
    // Garante que nenhum frame capturado seja perdido silenciosamente.
    Falcor::Logger.log_info("[Pipeline] Drenando frames restantes...");

    RawFrame frame;
    while (m_buffer.consume(frame, std::chrono::milliseconds(0)))
    {
        std::lock_guard<std::mutex> lock(m_configMutex);
        for (auto& step : m_steps)
            frame = step->process(std::move(frame));

        if (m_outputHandler)
            m_outputHandler(std::move(frame));
    }

    Falcor::Logger.log_info("[Pipeline] Loop encerrado.");
}
