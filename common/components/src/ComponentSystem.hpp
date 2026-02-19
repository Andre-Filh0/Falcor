#pragma once
#include <iostream>
#include <unordered_map>
#include <cstdint>
#include <memory>
#include <mutex> 
#include <typeindex>

namespace Falcor::Components {

    // Interface Base: Define o contrato para todos os componentes
    class IComponent {
    public:
        virtual ~IComponent() = default;

    protected:
        // Métodos protegidos para evitar chamadas manuais fora do Registry
        virtual void initialize() {}
        virtual void shutdown() {}

        friend class ComponentRegistry;
    };

    // Registry: Gerenciador Central de Ciclo de Vida e Alocação
    class ComponentRegistry {
    private:
        inline static std::unordered_map<size_t, std::shared_ptr<IComponent>> m_registry_table;
        inline static std::mutex m_registry_mutex;
        inline static size_t m_discovery_counter = 0;

    public:
        // Reserva memória no map para evitar re-hashing durante a execução no Kria
        static void reserve_memory(size_t pool_size);
        
        // Chamado automaticamente pelo CRTP para contar classes detectadas
        static void notify_discovery();
        static size_t get_discovered_count();

        // Garante instância única (Singleton) por tipo
        template <typename TComponent>
        static TComponent* get_or_create() {
            static_assert(std::is_base_of_v<IComponent, TComponent>, "T deve herdar de IComponent");

            const size_t component_id = typeid(TComponent).hash_code();

            // Lock integral para garantir criação atômica e evitar race conditions
            std::lock_guard<std::mutex> lock(m_registry_mutex);
            
            auto& component_ptr = m_registry_table[component_id];
            if (!component_ptr) {
                component_ptr = std::make_shared<TComponent>();
                component_ptr->initialize(); // Acesso via friend class
            }

            return static_cast<TComponent*>(component_ptr.get());
        }

        // Finaliza todos os componentes e limpa o Heap
        static void shutdown_all();
    };

    // Classe CRTP: Automatiza o registro e descoberta de componentes
    template<typename T>
    class CComponent : public IComponent {
    public:
        CComponent() { (void)s_registered; }

    private:
        inline static bool s_registered = []() {
            ComponentRegistry::notify_discovery();
            return true;
        }();
    };

    // Atalho global para acesso rápido
    template <typename T>
    inline T* GetOrCreateComponent() {
        return ComponentRegistry::get_or_create<T>();
    }
}