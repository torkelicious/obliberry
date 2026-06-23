#pragma once


#include "json.hpp"
#include "Core/ResourceManager.h"
#include "ECS/Entity.h"
#include <unordered_map>
#include <functional>
#include <string>

// for asset linking
using ComponentDeserializer = std::function<void(Entity &, const nlohmann::json &, ResourceManager &)>;
using ComponentSerializer = std::function<void(Entity &, nlohmann::json &, ResourceManager &)>;

class EntityFactory {
public:
    static void RegisterDeserializers();

    static void RegisterSerializers();

    static const std::unordered_map<std::string, ComponentDeserializer> &GetDeserializers();

    static const std::unordered_map<std::string, ComponentSerializer> &GetSerializers();

    static void DeserializeEntity(Entity &entity, const nlohmann::json &entityData, ResourceManager &resources);

    static void SerializeEntity(Entity &entity, nlohmann::json &outEntityData, ResourceManager &resources);

private:
    static std::unordered_map<std::string, ComponentDeserializer> s_Deserializers;
    static std::unordered_map<std::string, ComponentSerializer> s_Serializers;
};
