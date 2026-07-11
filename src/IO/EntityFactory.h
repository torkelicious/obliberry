#pragma once


#include <nlohmann/json.hpp>
#include "Core/ResourceManager.h"
#include "ECS/Entity.h"
#include <unordered_map>
#include <functional>
#include <string>

// for asset linking
using ComponentDeserializer = std::function<void(ECS::Entity &, const nlohmann::json &, Core::ResourceManager &)>;
using ComponentSerializer = std::function<void(ECS::Entity &, nlohmann::json &, Core::ResourceManager &)>;

namespace IO {
    class EntityFactory {
    public:
        static void RegisterDeserializers();

        static void RegisterSerializers();

        static const std::unordered_map<std::string, ComponentDeserializer> &GetDeserializers();

        static const std::unordered_map<std::string, ComponentSerializer> &GetSerializers();

        static void DeserializeEntity(ECS::Entity &entity, const nlohmann::json &entityData, Core::ResourceManager &resources);

        static void SerializeEntity(ECS::Entity &entity, nlohmann::json &outEntityData, Core::ResourceManager &resources);

    private:
        static std::unordered_map<std::string, ComponentDeserializer> s_Deserializers;
        static std::unordered_map<std::string, ComponentSerializer> s_Serializers;
    };
} // namespace IO
