#pragma once


#include <memory>
#include <mutex>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Core {
    class IResourceCache {
    public:
        virtual ~IResourceCache() = default;
        virtual void ClearProjectResources() = 0;
    };

    // cache per resource type
    template <typename T> class ResourceCache : public IResourceCache {
    public:
        std::unordered_map<std::string, std::shared_ptr<T>> storage;

        // dont clear builting shaders etc
        void ClearProjectResources() override {
            std::erase_if(storage, [](const auto &kv) { return kv.first.rfind("[Engine]", 0) != 0; });
        }
    };

    class ResourceManager {
    public:
        // prevent copying / moving
        ResourceManager(const ResourceManager &) = delete;
        ResourceManager &operator=(const ResourceManager &) = delete;
        ResourceManager(ResourceManager &&) = delete;
        ResourceManager &operator=(ResourceManager &&) = delete;

        static ResourceManager &GetInstance() {
            static ResourceManager instance;
            return instance;
        }

        template <typename T, typename... Args> std::shared_ptr<T> Load(const std::string &key, Args &&...args) {
            std::lock_guard lock(m_Mutex);
            auto &cache = GetCache<T>();
            if (auto it = cache.storage.find(key); it != cache.storage.end()) {
                return it->second;
            }
            return cache.storage.emplace(key, std::make_shared<T>(std::forward<Args>(args)...)).first->second;
        }

        template <typename T> std::shared_ptr<T> Get(const std::string &key) {
            std::lock_guard lock(m_Mutex);
            auto &cache = GetCache<T>();

            auto it = cache.storage.find(key);

            if (it == cache.storage.end())
                return nullptr;

            return it->second;
        }

        template <typename T> std::string GetKey(std::shared_ptr<T> resource) {
            if (!resource)
                return "";

            std::lock_guard lock(m_Mutex);
            for (auto &cache = GetCache<T>(); const auto &[key, ptr] : cache.storage) {
                if (ptr == resource) {
                    return key;
                }
            }
            return "";
        }

        template <typename T> std::vector<std::pair<std::string, std::shared_ptr<T>>> GetAll() {
            std::lock_guard lock(m_Mutex);
            auto &storage = GetCache<T>().storage;
            return {storage.begin(), storage.end()};
        }


        template <typename T, typename Func> std::shared_ptr<T> LoadFromFactory(const std::string &key, Func &&factory) {
            {
                std::lock_guard lock(m_Mutex);
                auto &cache = GetCache<T>();
                if (auto it = cache.storage.find(key); it != cache.storage.end()) {
                    return it->second;
                }
            }
            auto created = factory();
            std::lock_guard lock(m_Mutex);
            auto &cache = GetCache<T>();
            if (auto it = cache.storage.find(key); it != cache.storage.end()) {
                return it->second; // another thread won the race
            }
            return cache.storage.emplace(key, std::move(created)).first->second;
        }

        template <typename T> std::shared_ptr<T> Register(const std::string &key, std::shared_ptr<T> resource) {
            std::lock_guard lock(m_Mutex);
            auto &cache = GetCache<T>();
            cache.storage[key] = std::move(resource);
            return cache.storage[key];
        }

        template <typename T> bool Unload(const std::string &key) {
            std::lock_guard lock(m_Mutex);
            return GetCache<T>().storage.erase(key) > 0;
        }

        void ClearProjectResources() {
            std::lock_guard lock(m_Mutex);
            for (auto &[typeIdx, cache] : m_Caches)
                cache->ClearProjectResources();
        }

    private:
        ResourceManager() = default;
        ~ResourceManager() = default;

        // runtime type tracking
        std::unordered_map<std::type_index, std::unique_ptr<IResourceCache>> m_Caches;
        std::mutex m_Mutex;

        template <typename T> ResourceCache<T> &GetCache() {
            const auto typeIdx = std::type_index(typeid(T));
            auto [it, inserted] = m_Caches.try_emplace(typeIdx);
            if (inserted) {
                it->second = std::make_unique<ResourceCache<T>>();
            }
            return *static_cast<ResourceCache<T> *>(it->second.get());
        }
    };
} // namespace Core
