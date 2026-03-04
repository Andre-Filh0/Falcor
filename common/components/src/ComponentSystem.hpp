#pragma once

#include <iostream>
#include <unordered_map>
#include <cstdint>
#include <memory>
#include <mutex> 
#include <typeindex>
#include <utility>
#include <type_traits>

namespace Falcor::Components {

    /**
     * @brief Interface Base para todos os componentes do Falcor.
     */
    class IComponent {
    public:
        virtual ~IComponent() = default;

    protected:
        // Métodos de ciclo de vida protegidos
        virtual void initialize() {}
        virtual void shutdown() {}

        friend class ComponentRegistry;
    };

    /**
     * @brief Gerenciador Central de Ciclo de Vida (Registry).
     * Responsável por garantir Singletons e gerenciar a memória no Kria KV260.
     */
    class ComponentRegistry {
    private:
        // Armazenamento central dos componentes
        inline static std::unordered_map<size_t, std::shared_ptr<IComponent>> m_registry_table;
        inline static std::mutex m_registry_mutex;
        inline static size_t m_discovery_counter = 0;

    public:
        /**
         * @brief Reserva memória para o mapa para evitar realocações em tempo de execução.
         */
        static void reserve_memory(size_t pool_size);
        
        static void notify_discovery();
        static size_t get_discovered_count();

        /**
         * @brief Obtém ou cria uma instância única de um componente.
         * Se argumentos forem passados, chama automaticamente o método 'configure'.
         */
        template <typename TComponent, typename... Args>
        static TComponent* get_or_create(Args&&... args) {
            static_assert(std::is_base_of_v<IComponent, TComponent>, "T deve herdar de IComponent");

            const size_t component_id = typeid(TComponent).hash_code();

            std::lock_guard<std::mutex> lock(m_registry_mutex);
            
            auto& component_ptr = m_registry_table[component_id];
            
            if (!component_ptr) {
                // 1. Instanciação
                auto instance = std::make_shared<TComponent>();

                // 2. Perfect Forwarding para o método configure (se houver argumentos)
                if constexpr (sizeof...(args) > 0) {
                    instance->configure(std::forward<Args>(args)...);
                }

                // 3. Inicialização automática (conforme solicitado)
                instance->initialize();

                component_ptr = instance;
            }

            return static_cast<TComponent*>(component_ptr.get());
        }

        /**
         * @brief Encerra todos os componentes na ordem correta.
         */
        static void shutdown_all();
    };

    /**
     * @brief Classe CRTP para registro automático de tipos.
     */
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

    /**
     * @brief Atalho global para acesso rápido aos componentes.
     */
    template <typename T, typename... Args>
    inline T* GetOrCreateComponent(Args&&... args) {
        return ComponentRegistry::get_or_create<T>(std::forward<Args>(args)...);
    }
}