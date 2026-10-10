#include "CustomDataSnapshot.h"
#include "ObSL/Parser/ast.h"

#include <functional>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <ObSL/Interpreter.h>
#include <variant>
#include <vector>

namespace Editor::Commands {
    CustomDataSnapshot CaptureCustomDataSnapshot(const std::unordered_map<std::string, ObSL::Value> &components) {

        CustomDataSnapshot snapshot;
        std::unordered_map<const ObSL::GCObject *, std::size_t> captured;

        std::function<CustomDataValue(const ObSL::Value &)> captureValue;

        captureValue = [&](const ObSL::Value &source) -> CustomDataValue {
            return std::visit([&](const auto &value) -> CustomDataValue {
                using T = std::decay_t<decltype(value)>;

                if constexpr (std::is_same_v<T, ObSL::ObSLCallable *>) {
                    throw std::runtime_error("Undo cannot snapshot custom script functions");

                } else if constexpr (std::is_same_v<T, ObSL::ObSLArray *> || std::is_same_v<T, ObSL::ObSLObject *>) {

                    if (!value) {
                        return std::monostate{};
                    }

                    const ObSL::GCObject *object = value;

                    if (const auto it = captured.find(object); it != captured.end()) {
                        return CustomDataReference{it->second};
                    }

                    const std::size_t index = snapshot.nodes.size();
                    captured.emplace(object, index);

                    if constexpr (std::is_same_v<T, ObSL::ObSLArray *>) {

                        snapshot.nodes.emplace_back(CustomDataArray{});

                        CustomDataArray array;
                        array.elements.reserve(value->elements.size());

                        for (const auto &element : value->elements) {
                            array.elements.push_back(captureValue(element));
                        }

                        snapshot.nodes[index] = std::move(array);

                    } else {
                        snapshot.nodes.emplace_back(CustomDataObject{});

                        CustomDataObject objectData;
                        objectData.fields.reserve(value->fields.size());

                        for (const auto &[name, field] : value->fields) {
                            objectData.fields.emplace(name, captureValue(field));
                        }

                        snapshot.nodes[index] = std::move(objectData);
                    }

                    return CustomDataReference{index};

                } else {
                    return value;
                }
            }, source);
        };

        snapshot.components.reserve(components.size());

        for (const auto &[name, value] : components) {
            snapshot.components.emplace(name, captureValue(value));
        }

        return snapshot;
    }


    std::unordered_map<std::string, ObSL::Value> RestoreCustomDataSnapshot(const CustomDataSnapshot &snapshot, ObSL::Interpreter &interpreter, ObSL::GCProtectScope &scope) {
        if (scope.interpreter != &interpreter) {
            throw std::invalid_argument("Custom data protect scope belongs to another interpreter.");
        }

        std::vector<ObSL::Value> nodes;
        nodes.reserve(snapshot.nodes.size());

        for (const auto &node : snapshot.nodes) {
            ObSL::Value value = std::visit([&](const auto &saved) -> ObSL::Value {
                using T = std::decay_t<decltype(saved)>;
                if constexpr (std::is_same_v<T, CustomDataArray>) {
                    return interpreter.gc.allocate<ObSL::ObSLArray>();
                } else {
                    return interpreter.gc.allocate<ObSL::ObSLObject>();
                }
            }, node);
            scope.protect(value);
            nodes.push_back(value);
        }

        const auto restoreValue = [&](const CustomDataValue &saved) -> ObSL::Value {
            return std::visit([&](const auto &value) -> ObSL::Value {
                using T = std::decay_t<decltype(value)>;
                if constexpr (std::is_same_v<T, CustomDataReference>) {
                    if (value.index >= nodes.size()) {
                        throw std::runtime_error("invalid custom data snapshot reference");
                    }
                    return nodes[value.index];
                } else {
                    return value;
                }
            }, saved);
        };

        for (std::size_t i = 0; i < snapshot.nodes.size(); ++i) {

            std::visit([&](const auto &saved) {
                using T = std::decay_t<decltype(saved)>;
                if constexpr (std::is_same_v<T, CustomDataArray>) {
                    auto *array = std::get<ObSL::ObSLArray *>(nodes[i]);
                    array->elements.reserve(saved.elements.size());
                    for (const auto &element : saved.elements) {
                        array->elements.push_back(restoreValue(element));
                    }
                } else {
                    auto *object = std::get<ObSL::ObSLObject *>(nodes[i]);
                    object->fields.reserve(saved.fields.size());
                    for (const auto &[name, field] : saved.fields) {
                        object->fields.emplace(name, restoreValue(field));
                    }
                }
            }, snapshot.nodes[i]);
        }

        std::unordered_map<std::string, ObSL::Value> components;
        components.reserve(snapshot.components.size());

        for (const auto &[name, value] : snapshot.components) {
            components.emplace(name, restoreValue(value));
        }
        return components;
    }


} // namespace Editor::Commands
