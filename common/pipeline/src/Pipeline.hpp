#pragma once

#include "IPipeline.hpp"
#include "DoubleBuffer.hpp"
#include "Logger.hpp"
#include <vector>
#include <thread>
#include <atomic>

/**
 * @brief Implementacao concreta da IPipeline.
 *
 * Roda uma thread dedicada que consome frames do DoubleBuffer,
 * passa por cada IPipelineStep em sequencia e entrega o resultado
 * ao FrameCallback definido pelo caller.
 */
class Pipeline : public IPipeline {
public:
    Pipeline()  = default;
    ~Pipeline() override { stop(); }

    void addStep(std::unique_ptr<IPipelineStep> step) override;
    void setOutputHandler(FrameCallback handler) override;

    void start() override;
    void stop()  override;
    bool isRunning() const override { return m_running.load(); }

    void pushFrame(RawFrame frame) override;

private:
    void processingLoop();

    // m_configMutex protege m_steps e m_outputHandler.
    // addStep() e setOutputHandler() podem ser chamados apos start() —
    // o mutex garante visibilidade correta na thread de processamento.
    mutable std::mutex                          m_configMutex;

    std::vector<std::unique_ptr<IPipelineStep>> m_steps;
    FrameCallback                               m_outputHandler;
    DoubleBuffer                                m_buffer;
    std::thread                                 m_thread;
    std::atomic<bool>                           m_running{false};
};
