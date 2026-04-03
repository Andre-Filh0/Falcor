#include "CPipelineComponent.hpp"
#include "Logger.hpp"

namespace Falcor::Components {

void CPipelineComponent::addStep(std::unique_ptr<IPipelineStep> step)
{
    m_pipeline.addStep(std::move(step));
}

void CPipelineComponent::setOutputHandler(IPipeline::FrameCallback handler)
{
    m_pipeline.setOutputHandler(std::move(handler));
}

void CPipelineComponent::pushFrame(RawFrame frame)
{
    m_pipeline.pushFrame(std::move(frame));
}

bool CPipelineComponent::isRunning() const
{
    return m_pipeline.isRunning();
}

void CPipelineComponent::initialize()
{
    Falcor::Logger.log_info("[CPipelineComponent] Iniciando pipeline...");
    m_pipeline.start();
}

void CPipelineComponent::shutdown()
{
    Falcor::Logger.log_info("[CPipelineComponent] Encerrando pipeline...");
    m_pipeline.stop();
}

} // namespace Falcor::Components
