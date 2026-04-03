#pragma once

#include <iostream>
#include <unordered_map>
#include <cstdint>
#include <memory>
#include <mutex>
#include <atomic>
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
     * Responsável por garantir Singletons e gerenciar a memória no Kria KR260.
     */
    class ComponentRegistry {
    private:
        // BUG FIX #9: Use std::type_index as key to avoid typeid().hash_code() collisions.
        inline static std::unordered_map<std::type_index, std::shared_ptr<IComponent>> m_registry_table;
        inline static std::mutex m_registry_mutex;
        // BUG FIX #2: Use std::atomic to prevent data races during static initialisation.
        inline static std::atomic<size_t> m_discovery_counter{0};

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
            static_assert(std::is_base_of_v<IComponent, TComponent>, "TComponent must inherit from IComponent");

            // BUG FIX #9: Use std::type_index — guaranteed unique per type, no hash collisions.
            const std::type_index component_id{typeid(TComponent)};

            std::lock_guard<std::mutex> lock(m_registry_mutex);

            auto& component_ptr = m_registry_table[component_id];

            if (!component_ptr) {
                // 1. Instantiation
                auto instance = std::make_shared<TComponent>();

                // 2. Perfect-forward arguments to configure() if provided
                if constexpr (sizeof...(args) > 0) {
                    instance->configure(std::forward<Args>(args)...);
                }

                // 3. Automatic lifecycle initialisation
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
     * @brief CRTP base class for automatic component type registration.
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
     * @brief Global shortcut for fast component access.
     */
    template <typename T, typename... Args>
    inline T* GetOrCreateComponent(Args&&... args) {
        return ComponentRegistry::get_or_create<T>(std::forward<Args>(args)...);
    }

} // namespace Falcor::Components