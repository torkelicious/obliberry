#ifndef OBLIBERRY_RESOURCEMANAGER_H
#define OBLIBERRY_RESOURCEMANAGER_H
#include <functional>
#include <vector>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

class ResourceManager {
public:
    /* todo:
     * implement proper handles stuff rather than this jank
     */


    ResourceManager() = delete;

    ~ResourceManager() = delete;

    static void Shutdown() {
        for (auto &cleanup: CleanupList()) {
            cleanup();
        }
    }

    template<typename T, typename... Args>
    static T *Load(const std::string &key, Args &&... args) {
        auto &storage = GetStorage<T>();
        auto it = storage.find(key);
        if (it != storage.end())
            return it->second.get();
        auto resource =
                std::make_unique<T>(std::forward<Args>(args)...);
        T *ptr = resource.get();
        storage[key] = std::move(resource);
        return ptr;
    }

    template<typename T, typename Func>
    static T *LoadWithInit(const std::string &key, Func &&init) {
        auto *obj = Load<T>(key);
        init(obj);
        return obj;
    }

    template<typename T, typename Func>
    static T *LoadFromFactory(const std::string &key, Func &&factory) {
        auto &storage = GetStorage<T>();

        auto it = storage.find(key);
        if (it != storage.end())
            return it->second.get();

        auto resource = factory(); // returns std::unique_ptr<T>

        T *ptr = resource.get();

        storage[key] = std::move(resource);

        return ptr;
    }

    // unsafe
    // will create dangling pointers if doing something like
    /*
      Texture *Tex = ResourceManager::Load<Texture>("Tex");
      ResourceManager::Delete<Texture>("Tex")

        // *Tex is now a dangling pointer :-P
    */
    template<typename T>
    static bool Delete(const std::string &key) {
        auto &storage = GetStorage<T>();

        auto it = storage.find(key);
        if (it == storage.end())
            return false;

        storage.erase(it);
        return true;
    }

private:
    template<typename T>
    static std::unordered_map<std::string, std::unique_ptr<T> > &GetStorage() {
        static std::unordered_map<
            std::string,
            std::unique_ptr<T>
        > storage;

        static bool registered = [] {
            CleanupList().push_back([] {
                storage.clear();
            });

            return true;
        }();

        (void) registered;

        return storage;
    }

    static std::vector<std::function<void()> > &CleanupList() {
        static std::vector<std::function<void()> > list;
        return list;
    }
};


#endif //OBLIBERRY_RESOURCEMANAGER_H
