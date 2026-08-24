#pragma once

#include "ECS/Registry.h"
#include <ObSL/Interpreter.h>

#include <cstdint>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>
#include <utility>

namespace Scripting {
    class EntityWrapperCache {
    public:
        enum class Kind : uint8_t {
            Entity = 0,
            Transform,
            PointLight,
            Movement,
            MapState,
            DirectionalTexture,
            BillboardTag,
            DestroyTag,
            ParticleEmitter,
        };

        struct Key {
            ECS::EntityID id;
            Kind kind;

            bool operator==(const Key &other) const { return id == other.id && kind == other.kind; }
        };

        struct KeyHash {
            size_t operator()(const Key &k) const { return std::hash<uint64_t>{}((static_cast<uint64_t>(k.id) << 8) | static_cast<uint64_t>(k.kind)); }
        };

        class Cache {
        public:
            explicit Cache(ObSL::Interpreter *interp) : m_Interp(interp) {}

            template <typename Validate> ObSL::ObSLObject *Find(const ECS::Registry &registry, const ECS::EntityID id, const Kind kind, Validate &&valid) {
                const auto it = m_Entries.find({id, kind});
                if (it == m_Entries.end())
                    return nullptr;
                if (it->second.registry != &registry || !valid()) {
                    if (it->second.wrapper)
                        m_Interp->gc.remove_root(it->second.wrapper);
                    m_Entries.erase(it);
                    return nullptr;
                }
                return it->second.wrapper;
            }

            void Store(const ECS::Registry &registry, const ECS::EntityID id, const Kind kind, ObSL::ObSLObject *wrapper) {
                const auto it = m_Entries.find({id, kind});
                if (it != m_Entries.end() && it->second.wrapper && it->second.wrapper != wrapper)
                    m_Interp->gc.remove_root(it->second.wrapper);
                m_Interp->gc.add_root(wrapper);
                m_Entries[{id, kind}] = Entry{wrapper, &registry};
            }

            void Clear() {
                for (const auto &entry : m_Entries | std::views::values) {
                    if (entry.wrapper)
                        m_Interp->gc.remove_root(entry.wrapper);
                }
                m_Entries.clear();
            }

        private:
            struct Entry {
                ObSL::ObSLObject *wrapper = nullptr;
                const ECS::Registry *registry = nullptr;
            };

            ObSL::Interpreter *m_Interp;
            std::unordered_map<Key, Entry, KeyHash> m_Entries;
        };

        static void RegisterInterpreter(ObSL::Interpreter *interp) {
            std::unique_lock lock(g_MapMutex);
            g_Caches.try_emplace(interp, Cache(interp));
        }

        static Cache *Get(ObSL::Interpreter *interp) {
            std::shared_lock lock(g_MapMutex);
            const auto it = g_Caches.find(interp);
            return it != g_Caches.end() ? &it->second : nullptr;
        }

        static void ClearAll() {
            std::shared_lock lock(g_MapMutex);
            for (auto &cache : g_Caches | std::views::values)
                cache.Clear();
        }

    private:
        inline static std::shared_mutex g_MapMutex;
        inline static std::unordered_map<ObSL::Interpreter *, Cache> g_Caches;
    };
} // namespace Scripting
