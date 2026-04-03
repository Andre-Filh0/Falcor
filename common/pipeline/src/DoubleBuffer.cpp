#include "DoubleBuffer.hpp"

void DoubleBuffer::produce(RawFrame frame)
{
    std::unique_lock<std::mutex> lock(m_mutex);

    // Backpressure: bloqueia ate que haja pelo menos um slot livre
    m_notFull.wait(lock, [this] { return m_count < 2; });

    m_slots[m_tail] = std::move(frame);
    m_tail = (m_tail + 1) % 2;
    ++m_count;

    m_notEmpty.notify_one();
}

bool DoubleBuffer::consume(RawFrame& outFrame, std::chrono::milliseconds timeout)
{
    std::unique_lock<std::mutex> lock(m_mutex);

    // Acorda se ha frame disponivel OU se wakeup() foi chamado (encerramento).
    if (!m_notEmpty.wait_for(lock, timeout, [this] { return m_count > 0 || m_wakeup; }))
        return false;

    // Wakeup sem frames pendentes: sinaliza encerramento ao consumidor.
    if (m_count == 0)
        return false;

    outFrame = std::move(m_slots[m_head]);
    m_head = (m_head + 1) % 2;
    --m_count;

    m_notFull.notify_one();
    return true;
}

void DoubleBuffer::wakeup()
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_wakeup = true;
    }
    m_notEmpty.notify_all(); // acorda consume() imediatamente
}
