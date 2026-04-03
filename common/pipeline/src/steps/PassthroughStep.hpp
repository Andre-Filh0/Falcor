#pragma once

#include "IPipelineStep.hpp"
#include "Logger.hpp"

/**
 * @brief Step de debug que passa o frame sem modificar.
 *
 * Util para testar o fluxo da pipeline sem processamento real,
 * ou como placeholder enquanto outros steps sao desenvolvidos.
 */
class PassthroughStep : public IPipelineStep {
public:
    std::string_view name() const override { return "Passthrough"; }

    RawFrame process(RawFrame frame) override
    {
        Falcor::Logger.log_debug(
            "[PassthroughStep] Frame #{} passado sem modificacao ({} bytes).",
            frame.metadata.frameId,
            frame.data.size()
        );
        return frame;
    }
};
