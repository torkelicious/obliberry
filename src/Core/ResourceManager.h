#ifndef OBLIBERRY_RESOURCEMANAGER_H
#define OBLIBERRY_RESOURCEMANAGER_H
#include <functional>
#include <vector>
#include <memory>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <utility>

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
        auto it = cache.storage.find(key);
        if (it != cache.storage.end()) {
            return it->second;
        }

        auto resource = std::make_shared<T>(std::forward<Args>(args)...);
        cache.storage[key] = resource;
        return resource;
    }

    template<typename T, typename Func>
    std::shared_ptr<T> LoadFromFactory(const std::string &key, Func &&factory) {
        auto &cache = GetCache<T>();
        auto it = cache.storage.find(key);
        if (it != cache.storage.end()) {
            return it->second;
        }

        // return a unique_ptr<T> or shared_ptr<T>
        std::shared_ptr<T> resource = factory();
        cache.storage[key] = resource;
        return resource;
    }

    template<typename T>
    bool Unload(const std::string &key) {
        auto &cache = GetCache<T>();
        auto it = cache.storage.find(key);
        if (it == cache.storage.end()) {
            return false;
        }
        cache.storage.erase(it);
        return true;
        // no longer dangly dangly :)
    }

private:
    // runtime type tracking
    std::unordered_map<std::type_index, std::unique_ptr<IResourceCache> > m_Caches;

    template<typename T>
    ResourceCache<T> &GetCache() {
        std::type_index typeIdx(typeid(T));
        auto it = m_Caches.find(typeIdx);
        if (it == m_Caches.end()) {
            auto newCache = std::make_unique<ResourceCache<T> >();
            auto *cachePtr = newCache.get();
            m_Caches[typeIdx] = std::move(newCache);
            return *cachePtr;
        }
        return *static_cast<ResourceCache<T> *>(it->second.get());
    }
};

#endif //OBLIBERRY_RESOURCEMANAGER_H
