#include "ComponentSystem.hpp"

namespace Falcor::Components {

    void ComponentRegistry::reserve_memory(size_t pool_size) {
        std::lock_guard<std::mutex> lock(m_registry_mutex);
        m_registry_table.reserve(pool_size);
        std::cout << "[Falcor] Memory pool reserved for " << pool_size << " components." << std::endl;
    }

    void ComponentRegistry::notify_discovery() {
        m_discovery_counter++;
    }

    size_t ComponentRegistry::get_discovered_count() {
        return m_discovery_counter;
    }

    void ComponentRegistry::shutdown_all() {
        std::lock_guard<std::mutex> lock(m_registry_mutex);
        
        std::cout << "[Falcor] Registry: Iniciando limpeza segura do sistema..." << std::endl;

        for (auto it = m_registry_table.begin(); it != m_registry_table.end(); ) {
            auto& instance = it->second;

            if (instance) {
                // Se o use_count > 1, existe um ponteiro perdido em outra parte do código (ex: main ou threads)
                if (instance.use_count() > 1) {
                    std::cout << "[Falcor] Alerta: '" << typeid(*instance).name() 
                              << "' possui " << (instance.use_count() - 1) 
                              << " referencia(s) externa(s) ativa(s)!" << std::endl;
                }

                instance->shutdown();
            }
            
            // Remove a referência do mapa. 
            // Se esta for a última referência, o destruidor da classe é chamado AGORA.
            it = m_registry_table.erase(it);
        }
        
        std::cout << "[Falcor] ComponentRegistry: Shutdown completo. Heap limpo." << std::endl;
    }
}