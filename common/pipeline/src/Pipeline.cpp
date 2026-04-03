#include "Pipeline.hpp"

void Pipeline::addStep(std::unique_ptr<IPipelineStep> step)
{
    if (m_running.load())
    {
        Falcor::Logger.log_error("[Pipeline] Nao e possivel adicionar steps com a pipeline rodando.");
        return;
    }
    Falcor::Logger.log_info("[Pipeline] Step adicionado: '{}'.", step->name());
    m_steps.push_back(std::move(step));
}

void Pipeline::setOutputHandler(FrameCallback handler)
{
    m_outputHandler = std::move(handler);
}

void Pipeline::start()
{
    if (m_running.load()) return;

    Falcor::Logger.log_info("[Pipeline] Iniciando ({} steps).", m_steps.size());

    for (auto& step : m_steps)
        step->initialize();

    m_running.store(true);
    m_thread = std::thread(&Pipeline::processingLoop, this);
}

void Pipeline::stop()
{
    if (!m_running.load()) return;

    Falcor::Logger.log_info("[Pipeline] Encerrando...");
    m_running.store(false);

    if (m_thread.joinable())
        m_thread.join();

    for (auto& step : m_steps)
        step->shutdown();

    Falcor::Logger.log_info("[Pipeline] Encerrada.");
}

void Pipeline::pushFrame(RawFrame frame)
{
    if (!m_running.load()) return;
    // Bloqueia se buffer cheio (backpressure da camera)
    m_buffer.produce(std::move(frame));
}

void Pipeline::processingLoop()
{
    Falcor::Logger.log_info("[Pipeline] Thread de processamento iniciada.");

    while (m_running.load())
    {
        RawFrame frame;

        // Aguarda ate 100ms por um frame antes de verificar m_running
        if (!m_buffer.consume(frame, std::chrono::milliseconds(100)))
            continue;

        // Passa pelo chain de steps em sequencia
        for (auto& step : m_steps)
        {
            frame = step->process(std::move(frame));
        }

        // Entrega o frame final ao handler
        if (m_outputHandler)
            m_outputHandler(std::move(frame));
    }

    Falcor::Logger.log_info("[Pipeline] Thread de processamento encerrada.");
}
