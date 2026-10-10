#include "CustomDataSnapshot.h"

#include <functional>
#include <stdexcept>
#include <type_traits>
#include <utility>

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
} // namespace Editor::Commands
