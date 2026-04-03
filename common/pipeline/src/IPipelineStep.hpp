#pragma once

#include "interfaces/ICameraComponent.hpp"
#include <string_view>

/**
 * @brief Interface para uma etapa da pipeline de processamento.
 *
 * Cada etapa recebe um RawFrame, processa-o e retorna o frame modificado.
 * As etapas sao encadeadas em sequencia pela Pipeline.
 *
 * Uso tipico:
 *   class MinhaEtapa : public IPipelineStep {
 *       std::string_view name() const override { return "MinhaEtapa"; }
 *       RawFrame process(RawFrame frame) override { ... return frame; }
 *   };
 */
class IPipelineStep {
public:
    virtual ~IPipelineStep() = default;

    /** Nome da etapa — usado em logs e diagnostico. */
    virtual std::string_view name() const = 0;

    /**
     * @brief Processa o frame e retorna o resultado.
     *
     * O frame e passado por move — a etapa assume ownership e retorna
     * o frame (modificado ou nao). Nunca retorna um frame vazio.
     */
    virtual RawFrame process(RawFrame frame) = 0;

    /** Chamado uma vez antes do primeiro frame. */
    virtual void initialize() {}

    /** Chamado ao encerrar a pipeline. */
    virtual void shutdown() {}
};
