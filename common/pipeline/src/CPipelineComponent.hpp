#pragma once

#include "ComponentSystem.hpp"
#include "IPipeline.hpp"
#include "Pipeline.hpp"

namespace Falcor::Components {

/**
 * @brief Componente de pipeline registrado no ComponentRegistry.
 *
 * Wrapper que expoe a Pipeline como um componente do sistema Falcor.
 * Ciclo de vida gerenciado pelo registry (initialize/shutdown).
 *
 * Uso tipico (startup):
 *   auto* pipe = CreateComponent<CPipelineComponent>();
 *   pipe->addStep(std::make_unique<PassthroughStep>());
 *   pipe->addStep(std::make_unique<VitisAIStep>("model.xmodel"));
 *   pipe->setOutputHandler([](RawFrame f) { ... });
 *   // initialize() e chamado automaticamente pelo CreateComponent
 *
 * Uso tipico (runtime):
 *   auto* pipe = GetComponent<CPipelineComponent>();
 *   if (pipe) pipe->pushFrame(std::move(frame));
 */
class CPipelineComponent : public CComponent<CPipelineComponent> {
public:
    CPipelineComponent()  = default;
    ~CPipelineComponent() override = default;

    void addStep(std::unique_ptr<IPipelineStep> step);
    void setOutputHandler(IPipeline::FrameCallback handler);
    void pushFrame(RawFrame frame);
    bool isRunning() const;

    void initialize() override;
    void shutdown()   override;

private:
    Pipeline m_pipeline;
};

} // namespace Falcor::Components
