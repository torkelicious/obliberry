#pragma once

#include <mutex>
#include <vector>

#include "Scripting/SmallFunction.h"

namespace UI {
    class UISystem;
}

namespace Scripting {
    class UICommandBuffer {
    public:
        void push(SmallFunction<void(UI::UISystem &)> command) {
            std::lock_guard lock(m_Mutex);
            m_Commands.push_back(std::move(command));
        }

        void flush(UI::UISystem &uiSystem) {
            std::vector<SmallFunction<void(UI::UISystem &)>> commands;
            {
                std::lock_guard lock(m_Mutex);
                commands.swap(m_Commands);
            }
            for (auto &cmd : commands) {
                cmd(uiSystem);
            }
        }

    private:
        std::vector<SmallFunction<void(UI::UISystem &)>> m_Commands;
        std::mutex m_Mutex;
    };
} // namespace Scripting
