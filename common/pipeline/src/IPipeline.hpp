#pragma once

#include "interfaces/ICameraComponent.hpp"
#include "IPipelineStep.hpp"
#include <memory>
#include <functional>

/**
 * @brief Interface da pipeline de processamento de frames.
 *
 * A pipeline encadeia IPipelineStep em sequencia.
 * Roda em thread dedicada — o caller nao bloqueia em pushFrame
 * (exceto se o double buffer estiver cheio, aplicando backpressure).
 *
 * Ciclo de vida:
 *   addStep(...)        — antes de start()
 *   setOutputHandler()  — antes de start()
 *   start()             — lanca a thread de processamento
 *   pushFrame(frame)    — chamado pela camera a cada frame capturado
 *   stop()              — encerra a thread
 */
class IPipeline {
public:
    virtual ~IPipeline() = default;

    using FrameCallback = std::function<void(RawFrame)>;

    /**
     * @brief Adiciona uma etapa ao final da cadeia de processamento.
     * Deve ser chamado antes de start().
     */
    virtual void addStep(std::unique_ptr<IPipelineStep> step) = 0;

    /**
     * @brief Define o handler chamado com o frame final (apos todos os steps).
     * Opcional — se nao definido, o frame e descartado apos o ultimo step.
     */
    virtual void setOutputHandler(FrameCallback handler) = 0;

    /** Inicia a thread de processamento. */
    virtual void start() = 0;

    /** Para a thread de processamento e aguarda conclusao. */
    virtual void stop() = 0;

    virtual bool isRunning() const = 0;

    /**
     * @brief Empurra um frame para o double buffer.
     *
     * Chamado pela camera a cada frame capturado.
     * Bloqueia se o buffer estiver cheio (backpressure — garante que
     * a camera nao avance mais de 1 frame na frente da pipeline).
     */
    virtual void pushFrame(RawFrame frame) = 0;
};
