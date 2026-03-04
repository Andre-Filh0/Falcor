#include "ComponentSystem.hpp"

namespace Falcor::Components {

    void ComponentRegistry::reserve_memory(size_t pool_size) {
        std::lock_guard<std::mutex> lock(m_registry_mutex);
        m_registry_table.reserve(pool_size);
        std::cout << "[Falcor] Registry: Pool de memoria reservado para " << pool_size << " componentes." << std::endl;
    }

    void ComponentRegistry::notify_discovery() {
        // Incremento atômico implícito por ser chamado no contexto de inicialização estática
        m_discovery_counter++;
    }

    size_t ComponentRegistry::get_discovered_count() {
        return m_discovery_counter;
    }

    void ComponentRegistry::shutdown_all() {
        std::lock_guard<std::mutex> lock(m_registry_mutex);
        
        std::cout << "[Falcor] Registry: Iniciando limpeza segura do sistema..." << std::endl;

        // Itera e encerra cada componente
        for (auto it = m_registry_table.begin(); it != m_registry_table.end(); ) {
            auto& instance = it->second;

            if (instance) {
                // Log de aviso caso existam referências perdidas (Dangling pointers)
                if (instance.use_count() > 1) {
                    std::cout << "[Falcor] Alerta: '" << typeid(*instance).name() 
                              << "' ainda possui " << (instance.use_count() - 1) 
                              << " referencia(s) externa(s) ativa(s)!" << std::endl;
                }

                // Chama o shutdown virtual da implementação
                instance->shutdown();
            }
            
            // Remove do mapa e libera a memória
            it = m_registry_table.erase(it);
        }
        
        std::cout << "[Falcor] ComponentRegistry: Shutdown completo. Memoria liberada." << std::endl;
    }

} // namespace Falcor::Components