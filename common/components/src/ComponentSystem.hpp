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
#include <stdexcept>

namespace Falcor::Components {

    class IComponent {
    public:
        virtual ~IComponent() = default;

    protected:
        virtual void initialize() {}
        virtual void shutdown() {}

        friend class ComponentRegistry;
    };

    class ComponentRegistry {
    private:
        // BUG FIX #9: std::type_index como chave — sem colisoes de hash.
        inline static std::unordered_map<std::type_index, std::shared_ptr<IComponent>> m_registry_table;
        inline static std::mutex m_registry_mutex;
        // BUG FIX #2: atomic para evitar data race na inicializacao estatica.
        inline static std::atomic<size_t> m_discovery_counter{0};

    public:
        static void reserve_memory(size_t pool_size);
        static void notify_discovery();
        static size_t get_discovered_count();

        /**
         * @brief Cria, configura (se args), inicializa e registra o componente.
         *
         * Ciclo de vida completo: instanciacao -> configure() -> initialize() -> registry.
         * Lanca std::logic_error se o componente ja foi criado anteriormente.
         *
         * @throws std::logic_error  se o componente ja existe no registry.
         * @throws qualquer excecao lancada por initialize() (ex: falha de hardware).
         */
        template <typename TComponent, typename... Args>
        static TComponent* create(Args&&... args) {
            static_assert(std::is_base_of_v<IComponent, TComponent>,
                "TComponent precisa herdar de IComponent");

            const std::type_index id{typeid(TComponent)};
            std::lock_guard<std::mutex> lock(m_registry_mutex);

            if (m_registry_table.count(id))
                throw std::logic_error(
                    std::string("ComponentRegistry::create — componente ja existe: ") +
                    typeid(TComponent).name());

            auto instance = std::make_shared<TComponent>();

            if constexpr (sizeof...(args) > 0)
                instance->configure(std::forward<Args>(args)...);

            instance->initialize();

            m_registry_table[id] = instance;
            return static_cast<TComponent*>(instance.get());
        }

        /**
         * @brief Busca um componente ja registrado.
         *
         * Nao tem efeito colateral — nao cria, nao inicializa.
         * Retorna nullptr se o componente nao existir.
         */
        template <typename TComponent>
        static TComponent* get() {
            static_assert(std::is_base_of_v<IComponent, TComponent>,
                "TComponent precisa herdar de IComponent");

            const std::type_index id{typeid(TComponent)};
            std::lock_guard<std::mutex> lock(m_registry_mutex);

            auto it = m_registry_table.find(id);
            if (it == m_registry_table.end())
                return nullptr;

            return static_cast<TComponent*>(it->second.get());
        }

        /**
         * @brief Encerra todos os componentes na ordem correta.
         */
        static void shutdown_all();
    };

    /**
     * @brief CRTP base — notifica o registry na primeira instanciacao do tipo.
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

    // ----------------------------------------------------------------
    // API publica — funcoes livres para uso no codigo da aplicacao
    // ----------------------------------------------------------------

    /**
     * @brief Cria e inicializa um componente. Lanca se ja existir.
     *
     * Uso tipico (startup):
     *   auto* cam = CreateComponent<CCameraComponent>(CameraType::DMV_CAMERA, config);
     */
    template <typename T, typename... Args>
    inline T* CreateComponent(Args&&... args) {
        return ComponentRegistry::create<T>(std::forward<Args>(args)...);
    }

    /**
     * @brief Busca um componente ja criado. Retorna nullptr se nao existir.
     *
     * Uso tipico (runtime):
     *   auto* cam = GetComponent<CCameraComponent>();
     *   if (cam) cam->getVideoBuffer();
     */
    template <typename T>
    inline T* GetComponent() {
        return ComponentRegistry::get<T>();
    }

} // namespace Falcor::Components
