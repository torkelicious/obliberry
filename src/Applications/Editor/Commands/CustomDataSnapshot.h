#pragma once

#include "ObSL/Parser/ast.h"
#include <cstddef>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>
#include <memory>
#include <stdexcept>
#include <utility>

namespace ObSL {
    class Interpreter;
    struct GCProtectScope;

} // namespace ObSL

namespace Editor::Commands {

    struct CustomDataReference {
        std::size_t index;
    };

    using CustomDataValue = std::variant<std::monostate, bool, double, std::string, CustomDataReference>;

    struct CustomDataArray {
        std::vector<CustomDataValue> elements;
    };

    struct CustomDataObject {
        std::unordered_map<std::string, CustomDataValue> fields;
    };

    using CustomDataNode = std::variant<CustomDataArray, CustomDataObject>;

    struct CustomDataSnapshot {
        std::unordered_map<std::string, CustomDataValue> components;
        std::vector<CustomDataNode> nodes;
    };

    [[nodiscard]] CustomDataSnapshot CaptureCustomDataSnapshot(const std::unordered_map<std::string, ObSL::Value> &components);

    [[nodiscard]] std::unordered_map<std::string, ObSL::Value> RestoreCustomDataSnapshot(const CustomDataSnapshot &snapshot, ObSL::Interpreter &interpreter, const ObSL::GCProtectScope &scope);


} // namespace Editor::Commands
