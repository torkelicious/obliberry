#pragma once


#include <memory>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <utility>

namespace Core {
    class IResourceCache {
    public:
        virtual ~IResourceCache() = default;
    };

    // cache per resource type
    template<typename T>
    class ResourceCache : public IResourceCache {
    public:
        std::unordered_map<std::string, std::shared_ptr<T> > storage;
    };

    class ResourceManager {
    public:
        ResourceManager() = default;

        ~ResourceManager() = default;

        // prevent copying
        ResourceManager(const ResourceManager &) = delete;

        ResourceManager &operator=(const ResourceManager &) = delete;


        template<typename T, typename... Args>
        std::shared_ptr<T> Load(const std::string &key, Args &&... args) {
            auto &cache = GetCache<T>();
            if (auto it = cache.storage.find(key); it != cache.storage.end()) {
                return it->second;
            }
            return cache.storage.emplace(key, std::make_shared<T>(std::forward<Args>(args)...)).first->second;
        }

        template<typename T>
        std::shared_ptr<T> Get(const std::string &key) {
            auto &cache = GetCache<T>();

            auto it = cache.storage.find(key);

            if (it == cache.storage.end())
                return nullptr;

            return it->second;
        }

        template<typename T>
        std::string GetKey(std::shared_ptr<T> resource) {
            if (!resource) return "";

            for (auto &cache = GetCache<T>(); const auto &[key, ptr]: cache.storage) {
                if (ptr == resource) {
                    return key;
                }
            }
            return "";
        }

        template<typename T>
        const std::unordered_map<std::string, std::shared_ptr<T> > &GetAll() {
            return GetCache<T>().storage;
        }


        template<typename T, typename Func>
        std::shared_ptr<T> LoadFromFactory(const std::string &key, Func &&factory) {
            auto &cache = GetCache<T>();
            if (auto it = cache.storage.find(key); it != cache.storage.end()) {
                return it->second;
            }
            return cache.storage.emplace(key, factory()).first->second;
        }

        template<typename T>
        bool Unload(const std::string &key) {
            return GetCache<T>().storage.erase(key) > 0;
        }

    private:
        // runtime type tracking
        std::unordered_map<std::type_index, std::unique_ptr<IResourceCache> > m_Caches;

        template<typename T>
        ResourceCache<T> &GetCache() {
            const auto typeIdx = std::type_index(typeid(T));
            auto [it, inserted] = m_Caches.try_emplace(typeIdx);
            if (inserted) {
                it->second = std::make_unique<ResourceCache<T> >();
            }
            return *static_cast<ResourceCache<T> *>(it->second.get());
        }
    };
} // namespace Core
