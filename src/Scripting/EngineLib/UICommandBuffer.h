#pragma once

#include <functional>
#include <mutex>
#include <vector>

namespace UI {
    class UISystem;
}

namespace Scripting {
    class UICommandBuffer {
    public:
        void push(std::function<void(UI::UISystem &)> command) {
            std::lock_guard lock(m_Mutex);
            m_Commands.push_back(std::move(command));
        }

        void flush(UI::UISystem &uiSystem) {
            std::vector<std::function<void(UI::UISystem &)>> commands;
            {
                std::lock_guard lock(m_Mutex);
                commands.swap(m_Commands);
            }
            for (auto &cmd : commands) {
                cmd(uiSystem);
            }
        }

    private:
        std::vector<std::function<void(UI::UISystem &)>> m_Commands;
        std::mutex m_Mutex;
    };
} // namespace Scripting
