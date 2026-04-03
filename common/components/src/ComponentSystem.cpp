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

        std::cout << "[Falcor] Registry: starting safe system shutdown..." << std::endl;

        for (auto it = m_registry_table.begin(); it != m_registry_table.end(); ) {
            auto& instance = it->second;

            if (instance) {
                if (instance.use_count() > 1) {
                    std::cout << "[Falcor] Warning: '" << it->first.name()
                              << "' still has " << (instance.use_count() - 1)
                              << " external reference(s) alive!" << std::endl;
                }

                // BUG FIX #8: shutdown() is called here by the registry.
                // Components must NOT call shutdown() from their own destructor
                // to avoid double-shutdown. See CCameraComponent.
                instance->shutdown();
            }

            it = m_registry_table.erase(it);
        }

        std::cout << "[Falcor] ComponentRegistry: shutdown complete." << std::endl;
    }

} // namespace Falcor::Components