#include "ComponentSystem.hpp"

namespace Falcor::Components {

    void ComponentRegistry::reserve_memory(size_t pool_size) {
        std::lock_guard<std::mutex> lock(m_registry_mutex);
        m_registry_table.reserve(pool_size);
        std::cout << "[Falcor] Registry: memory pool reserved for " << pool_size << " components." << std::endl;
    }

    void ComponentRegistry::notify_discovery() {
        // BUG FIX #2: atomic fetch_add — safe to call from concurrent static initialisers.
        m_discovery_counter.fetch_add(1, std::memory_order_relaxed);
    }

    size_t ComponentRegistry::get_discovered_count() {
        return m_discovery_counter.load(std::memory_order_relaxed);
    }

    void ComponentRegistry::shutdown_all() {
        std::lock_guard<std::mutex> lock(m_registry_mutex);

        std::cout << "[Falcor] Registry: iniciando shutdown ordenado ("
                  << m_creation_order.size() << " componente(s))..." << std::endl;

        // Itera em ordem REVERSA de criacao:
        // o ultimo componente criado e encerrado primeiro.
        // Exemplo: Pipeline criada antes da Camera →
        //          Camera encerra primeiro (para de enviar frames),
        //          depois Pipeline (drena buffer e encerra thread).
        for (auto it = m_creation_order.rbegin(); it != m_creation_order.rend(); ++it)
        {
            auto mapIt = m_registry_table.find(*it);
            if (mapIt == m_registry_table.end())
                continue;

            auto& instance = mapIt->second;
            if (instance)
            {
                if (instance.use_count() > 1) {
                    std::cout << "[Falcor] Aviso: '"
                              << it->name()
                              << "' ainda tem " << (instance.use_count() - 1)
                              << " referencia(s) externa(s) viva(s)." << std::endl;
                }

                // O registry chama shutdown() — componentes NAO devem
                // chamar shutdown() no proprio destrutor (double-shutdown).
                instance->shutdown();
            }

            m_registry_table.erase(mapIt);
        }

        m_creation_order.clear();

        std::cout << "[Falcor] ComponentRegistry: shutdown completo." << std::endl;
    }

} // namespace Falcor::Components