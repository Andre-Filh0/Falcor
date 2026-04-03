#pragma once

#include "interfaces/ICameraComponent.hpp"
#include <array>
#include <mutex>
#include <condition_variable>
#include <chrono>

/**
 * @brief Double buffer thread-safe entre camera (produtor) e pipeline (consumidor).
 *
 * Implementado como fila bounded de tamanho 2:
 *   - Camera escreve no slot livre enquanto pipeline processa o outro.
 *   - Camera bloqueia somente se ambos os slots estiverem ocupados
 *     (backpressure: garante que nenhum frame e descartado silenciosamente).
 *   - Pipeline bloqueia ate que um frame esteja disponivel.
 *
 * Diagrama de estado:
 *
 *   [Camera] → produce(frame) → [Slot A] ←→ [Slot B] → consume() → [Pipeline]
 *
 *   count=0: camera pode escrever, pipeline bloqueia
 *   count=1: camera pode escrever, pipeline pode ler  (estado normal)
 *   count=2: camera bloqueia,      pipeline pode ler  (backpressure)
 */
class DoubleBuffer {
public:
    /**
     * @brief Deposita frame no buffer. Bloqueia se ambos os slots estiverem ocupados.
     * Thread-safe. Chamado pela camera.
     */
    void produce(RawFrame frame);

    /**
     * @brief Retira o frame mais antigo do buffer.
     * Bloqueia ate timeout se nao houver frame disponivel.
     * Thread-safe. Chamado pela pipeline.
     *
     * @return true se um frame foi retirado, false em timeout ou wakeup.
     */
    bool consume(RawFrame& outFrame, std::chrono::milliseconds timeout);

    /**
     * @brief Acorda o consumidor imediatamente (ex: ao encerrar a pipeline).
     *
     * Apos wakeup(), consume() retorna false assim que o buffer esvaziar,
     * sem esperar o timeout completo.
     */
    void wakeup();

private:
    std::array<RawFrame, 2> m_slots;
    int                     m_head{0};    // pipeline le daqui
    int                     m_tail{0};    // camera escreve aqui
    int                     m_count{0};   // frames disponiveis (0, 1 ou 2)
    bool                    m_wakeup{false}; // sinaliza encerramento

    std::mutex              m_mutex;
    std::condition_variable m_notEmpty;  // pipeline espera aqui
    std::condition_variable m_notFull;   // camera espera aqui (backpressure)
};
