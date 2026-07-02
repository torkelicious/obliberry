#pragma once

#include <functional>
#include <mutex>
#include <vector>

namespace ECS {
    class Registry;
}

namespace Scripting {
    // safe buffer of deferred registry mutations because thread safety yay
    class ScriptCommandBuffer {
    public:
        void push(std::function<void(ECS::Registry &)> command);

        void flush(ECS::Registry &registry);

    private:
        std::vector<std::function<void(ECS::Registry &)> > m_Commands;
        std::mutex m_Mutex;
    };
} // Scripting
