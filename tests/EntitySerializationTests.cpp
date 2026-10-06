#include <gtest/gtest.h>

#include "Core/ResourceManager.h"
#include "ECS/Components/ColliderComponent.h"
#include "ECS/Components/MovementComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Entity.h"
#include "ECS/Registry.h"
#include "ECS/Types.h"
#include "IO/Loaders/EntityFactory.h"
#include "TestingUtils.h"
#include "nlohmann/json.hpp"
#include "Core/Utils/UUID.h"
#include "ECS/Components/BillboardTagComponent.h"
#include "ECS/Components/DirectionalTextureComponent.h"
#include "ECS/Components/MaterialComponent.h"
#include "ECS/Components/MeshComponent.h"
#include "ECS/Components/ParticleEmitterComponent.h"
#include "ECS/Components/PointLightComponent.h"
#include "ECS/Components/ScriptComponent.h"
#include "ECS/Components/SpriteAnimatorComponent.h"
#include "ECS/Components/SpriteSheetComponent.h"
#include "ECS/Systems/Animation/Animation.h"
#include "IO/AnimationSerialization.h"
#include "Rendering/Types/Material.h"
#include "Rendering/Types/Mesh/Mesh.h"
#include "Rendering/Types/Texture/Texture.h"
#include "nlohmann/json_fwd.hpp"

#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

class EntitySerializationTests : public testing::Test {
protected:
    ECS::Registry sourceRegistry;
    ECS::Registry destinationRegistry;

    Core::ResourceManager &resources = Core::ResourceManager::GetInstance();

    std::vector<std::function<void()>> assetCleanup;

    void SetUp() override {
        IO::EntityFactory::RegisterSerializers();
        IO::EntityFactory::RegisterDeserializers();
    }

    void TearDown() override {
        for (const auto &cleanup : assetCleanup) {
            cleanup();
        }
    }

    ECS::Entity CreateEntity(ECS::Registry &registry) { return ECS::Entity{registry.CreateEntity(), &registry}; }

    nlohmann::json Serialize(ECS::Entity &entity) {
        nlohmann::json data;
        IO::EntityFactory::SerializeEntity(entity, data, resources);
        return data;
    }

    std::string GetUUID(const ECS::Entity &entity) { return entity.GetRegistry()->GetEntityUUID(static_cast<ECS::EntityID>(entity)); }

    template <typename T> std::string RegisterAsset(const std::shared_ptr<T> &asset) {
        const auto key = "serialization-test-" + Core::Utils::UUID::UUIDGenerator::Generate();
        resources.Register(key, asset);
        assetCleanup.emplace_back([this, key]() { resources.Unload<T>(key); });
        return key;
    }

    ECS::Entity Load(const nlohmann::json &data, const bool preserveUUID = false) {
        auto loaded = CreateEntity(destinationRegistry);
        IO::EntityFactory::DeserializeEntity(loaded, data, resources, preserveUUID);
        return loaded;
    }

    ECS::Entity RoundTrip(ECS::Entity &source) { return Load(nlohmann::json::parse(Serialize(source).dump())); }

    std::shared_ptr<Rendering::Texture> CreateTexture() {
        // CPU construction !!!
        return std::make_shared<Rendering::Texture>(34, 18, nullptr);
    }

    std::shared_ptr<Animation::SpriteAnimationSet> CreateAnimation(const std::shared_ptr<Rendering::Texture> &texture) {
        auto animation = std::make_shared<Animation::SpriteAnimationSet>();

        animation->sheet = std::make_shared<Rendering::SpriteSheet>();
        animation->sheet->texture = texture;
        animation->sheet->columns = 4;
        animation->sheet->rows = 2;
        animation->sheet->columnSpacing = 2;
        animation->sheet->rowSpacing = 2;

        animation->clips["idle"] = {{{2, 0.125f}, {0, 0.25f}, {3, 0.125f}}, true};
        animation->clips["once"] = {{{1, 0.5f}, {4, 0.25f}}, false};

        return animation;
    }

    static void ExpectSheet(const Rendering::SpriteSheet &actual, const Rendering::SpriteSheet &expected) {
        EXPECT_EQ(actual.texture, expected.texture);
        EXPECT_EQ(actual.columns, expected.columns);
        EXPECT_EQ(actual.rows, expected.rows);
        EXPECT_EQ(actual.columnSpacing, expected.columnSpacing);
        EXPECT_EQ(actual.rowSpacing, expected.rowSpacing);
    }
};


// Components should survive serialization and deserialization
// (there are some exceptions, but these components are expected to survive)
TEST_F(EntitySerializationTests, ComponentRoundtrip) {
    const std::string expectedName = "TestEntity";
    constexpr glm::vec3 expectedPosition{2.0f, 3.0f, 4.0f};
    constexpr glm::vec3 expectedRotation{0.25f, 0.5f, 0.75f};
    constexpr glm::vec3 expectedScale{2.0f, 3.0f, 4.0f};
    constexpr float expectedTimePerStep = 0.75f;

    constexpr ECS::Components::ColliderComponent expectedCollider{
            .shape = ECS::Components::ColliderShape::Cylinder,
            .orientation = ECS::Components::ColliderOrientation::Billboard,
            .offset = {0.25f, 0.5f, 0.75f},
            .size = {2.0f, 3.0f, 4.0f},
            .radius = 0.75f,
            .height = 2.0f,
            .isTrigger = true,
            .layer = 3,
            .mask = (1u << 1) | (1u << 31),
    };


    auto source = CreateEntity(sourceRegistry);
    source.SetName(expectedName);


    auto &transform = source.AddComponent<ECS::Components::TransformComponent>();

    transform.transform.SetPosition(expectedPosition);
    transform.transform.SetRotation(expectedRotation);
    transform.transform.SetScale(expectedScale);

    auto &movement = source.AddComponent<ECS::Components::MovementComponent>();
    movement.timePerStep = expectedTimePerStep;
    movement.autoMove = true;

    source.AddComponent<ECS::Components::ColliderComponent>(expectedCollider);

    const auto data = nlohmann::json::parse(Serialize(source).dump());
    auto loaded = CreateEntity(destinationRegistry);
    IO::EntityFactory::DeserializeEntity(loaded, data, resources);

    const auto *loadedTransform = loaded.GetComponent<ECS::Components::TransformComponent>();
    const auto *loadedMovement = loaded.GetComponent<ECS::Components::MovementComponent>();
    const auto *loadedCollider = loaded.GetComponent<ECS::Components::ColliderComponent>();

    ASSERT_NE(loadedTransform, nullptr);
    ASSERT_NE(loadedMovement, nullptr);
    ASSERT_NE(loadedCollider, nullptr);

    EXPECT_EQ(loaded.GetName(), expectedName);

    TestUtils::GLM_VecExpectFloat(loadedTransform->transform.GetPosition(), expectedPosition);
    TestUtils::GLM_VecExpectFloat(loadedTransform->transform.GetRotation(), expectedRotation);
    TestUtils::GLM_VecExpectFloat(loadedTransform->transform.GetScale(), expectedScale);

    EXPECT_FLOAT_EQ(loadedMovement->timePerStep, expectedTimePerStep);
    EXPECT_TRUE(loadedMovement->autoMove);

    EXPECT_EQ(*loadedCollider, expectedCollider);
}

// preserved uuids should be restored
TEST_F(EntitySerializationTests, PreserveUUIDRestoresSavedIdentity) {
    auto source = CreateEntity(sourceRegistry);
    const std::string savedUUID = GetUUID(source);
    const auto data = Serialize(source);

    auto loaded = CreateEntity(destinationRegistry);

    IO::EntityFactory::DeserializeEntity(loaded, data, resources, true);

    EXPECT_EQ(GetUUID(loaded), savedUUID);
}

// non-preserved should not keep uuid
TEST_F(EntitySerializationTests, WithoutPreserveKeepsNewIdentity) {
    auto source = CreateEntity(sourceRegistry);
    const auto data = Serialize(source);

    auto loaded = CreateEntity(destinationRegistry);
    const std::string generatedUUID = GetUUID(loaded);

    ASSERT_NE(generatedUUID, GetUUID(source));

    IO::EntityFactory::DeserializeEntity(loaded, data, resources, false);

    EXPECT_EQ(GetUUID(loaded), generatedUUID);
    EXPECT_NE(GetUUID(loaded), GetUUID(source));
}

// a fucked up collider should not break the transform or loading.
TEST_F(EntitySerializationTests, MalformedColliderKeepsTransformValid) {
    const auto data = nlohmann::json::parse(
            R"(
        {
            "components": {
                "TransformComponent": {
                    "position": [1, 2, 3]
                },
                "ColliderComponent": {
                    "version": 2,
                    "shape": "Box",
                    "size": [0, 1, 1]
                }
            }
        }
        )");

    auto loaded = CreateEntity(destinationRegistry);

    ASSERT_NO_THROW(IO::EntityFactory::DeserializeEntity(loaded, data, resources, true));
    EXPECT_TRUE(destinationRegistry.IsValid(static_cast<ECS::EntityID>(loaded)));
    EXPECT_FALSE(loaded.HasComponent<ECS::Components::ColliderComponent>());

    const auto *transform = loaded.GetComponent<ECS::Components::TransformComponent>();
    ASSERT_NE(transform, nullptr);
    TestUtils::GLM_VecExpectFloat(transform->transform.GetPosition(), glm::vec3{1.0f, 2.0f, 3.0f});
}

TEST_F(EntitySerializationTests, DuplicateUUIDKeepsGeneratedIdentity) {
    auto source = CreateEntity(sourceRegistry);
    const auto data = Serialize(source);
    const std::string savedUUID = GetUUID(source);

    auto first = CreateEntity(destinationRegistry);

    IO::EntityFactory::DeserializeEntity(first, data, resources, true);

    ASSERT_EQ(GetUUID(first), savedUUID);

    auto second = CreateEntity(destinationRegistry);
    const std::string secondOriginalUUID = GetUUID(second);

    IO::EntityFactory::DeserializeEntity(second, data, resources, true);

    EXPECT_TRUE(destinationRegistry.IsValid(static_cast<ECS::EntityID>(first)));
    EXPECT_TRUE(destinationRegistry.IsValid(static_cast<ECS::EntityID>(second)));

    EXPECT_EQ(GetUUID(first), savedUUID);
    EXPECT_EQ(GetUUID(second), secondOriginalUUID);
    EXPECT_NE(GetUUID(first), GetUUID(second));
}

TEST_F(EntitySerializationTests, PointLightAndBillboardRoundtrip) {
    auto src = CreateEntity(sourceRegistry);

    src.AddComponent<ECS::Components::BillboardTagComponent>();
    auto &light = src.AddComponent<ECS::Components::PointLightComponent>();

    light.SetColor({0.25f, 0.5f, 0.75f});
    light.SetRadius(12.0f);
    light.SetIntensity(2.5f);
    light.dirty = false;

    auto loaded = RoundTrip(src);

    EXPECT_TRUE(loaded.HasComponent<ECS::Components::BillboardTagComponent>());

    const auto *actual = loaded.GetComponent<ECS::Components::PointLightComponent>();
    ASSERT_NE(actual, nullptr);

    TestUtils::GLM_VecExpectFloat(actual->color, light.color);
    EXPECT_FLOAT_EQ(actual->radius, light.radius);
    EXPECT_FLOAT_EQ(actual->intensity, light.intensity);
    EXPECT_TRUE(actual->dirty);
}

TEST_F(EntitySerializationTests, ScriptPathsRoundtripInOrder) {
    const std::vector<std::vector<std::string>> cases{{"scripts/player.obsl"}, {"scripts/one.obsl"
                                                                                "scripts/two.obsl"
                                                                                "scripts/three.obsl"}};

    for (const auto &paths : cases) {
        SCOPED_TRACE(paths.size());

        auto src = CreateEntity(sourceRegistry);
        auto &script = src.AddComponent<ECS::Components::ScriptComponent>();
        script.slots.resize(paths.size());

        for (std::size_t i = 0; i < paths.size(); ++i) {
            script.slots[i].scriptPath = paths[i];
            script.slots[i].isInitialized = true;
            script.slots[i].resolvedPath = "/runtime/path.obsl";
            script.slots[i].source_code = "runtime source";
        }


        auto loaded = RoundTrip(src);
        const auto *actual = loaded.GetComponent<ECS::Components::ScriptComponent>();

        ASSERT_NE(actual, nullptr);
        ASSERT_EQ(actual->slots.size(), paths.size());

        for (std::size_t i = 0; i < paths.size(); ++i) {
            SCOPED_TRACE(i);

            EXPECT_EQ(actual->slots[i].scriptPath, paths[i]);
            EXPECT_FALSE(actual->slots[i].isInitialized);
            EXPECT_TRUE(actual->slots[i].resolvedPath.empty());
            EXPECT_TRUE(actual->slots[i].source_code.empty());
            EXPECT_TRUE(actual->slots[i].instance_envs.empty());
            EXPECT_TRUE(actual->slots[i].ast_nodes.empty());
        }
    }
}

TEST_F(EntitySerializationTests, ParticleConfigurationRoundtrip) {
    const nlohmann::json expected{{"maxParticles", 123}, {"emitRate", 12.5f}, {"lifetimeMin", 0.25f}, {"lifetimeMax", 3.0f}, {"velocityMin", {-2.0f, 1.0f, -3.0f}}, {"velocityMax", {4.0f, 5.0f, 6.0f}},
            {"gravity", {0.0f, -9.5f, 1.0f}}, {"sizeStartMin", 0.125f}, {"sizeStartMax", 0.75f}, {"sizeEndMin", 0.25f}, {"sizeEndMax", 1.5f}, {"rotationSpeedMin", -2.0f}, {"rotationSpeedMax", 4.0f},
            {"colorStart", {0.25f, 0.5f, 0.75f, 1.0f}}, {"colorEnd", {1.0f, 0.75f, 0.5f, 0.25f}}, {"isBillboard", true}, {"blendMode", 1}, {"renderOrder", 7}, {"shape", 2}};

    auto source = Load({{"components", {{"ParticleEmitterComponent", expected}}}});

    auto *emitter = source.GetComponent<ECS::Components::ParticleEmitterComponent>();

    ASSERT_NE(emitter, nullptr);

    emitter->emitterIndex = 5;
    emitter->aliveCount = 12;
    emitter->emitAccumulator = 0.5f;
    emitter->active = false;
    emitter->isDirty = false;

    const auto saved = Serialize(source);

    EXPECT_EQ(saved.at("components").at("ParticleEmitterComponent"), expected);

    auto loaded = RoundTrip(source);

    EXPECT_EQ(Serialize(loaded).at("components").at("ParticleEmitterComponent"), expected);

    const auto *actual = loaded.GetComponent<ECS::Components::ParticleEmitterComponent>();

    ASSERT_NE(actual, nullptr);

    EXPECT_EQ(actual->emitterIndex, -1);
    EXPECT_EQ(actual->aliveCount, 0);
    EXPECT_FLOAT_EQ(actual->emitAccumulator, 0.0f);
    EXPECT_TRUE(actual->active);
    EXPECT_TRUE(actual->isDirty);
}

TEST_F(EntitySerializationTests, MissingOptionalFieldsUseDefaults) {
    using namespace ECS::Components;

    nlohmann::json components = nlohmann::json::object();

    for (const char *name : {"TransformComponent", "MovementComponent", "MeshComponent", "MaterialComponent", "BillboardTagComponent", "DirectionalTextureComponent", "PointLightComponent", "ScriptComponent",
                 "ParticleEmitterComponent", "SpriteSheetComponent", "SpriteAnimatorComponent"}) {
        components[name] = nlohmann::json::object();
    }

    components["ColliderComponent"] = {{"version", 2}};

    auto loaded = Load({{"components", components}});

    const auto *transform = loaded.GetComponent<TransformComponent>();
    const auto *movement = loaded.GetComponent<MovementComponent>();
    const auto *mesh = loaded.GetComponent<MeshComponent>();
    const auto *material = loaded.GetComponent<MaterialComponent>();
    const auto *textures = loaded.GetComponent<DirectionalTextureComponent>();
    const auto *light = loaded.GetComponent<PointLightComponent>();
    const auto *script = loaded.GetComponent<ScriptComponent>();
    const auto *emitter = loaded.GetComponent<ParticleEmitterComponent>();
    const auto *sprite = loaded.GetComponent<SpriteSheetComponent>();
    const auto *animator = loaded.GetComponent<SpriteAnimatorComponent>();
    const auto *collider = loaded.GetComponent<ColliderComponent>();

    ASSERT_NE(transform, nullptr);
    ASSERT_NE(movement, nullptr);
    ASSERT_NE(mesh, nullptr);
    ASSERT_NE(material, nullptr);
    ASSERT_NE(textures, nullptr);
    ASSERT_NE(light, nullptr);
    ASSERT_NE(script, nullptr);
    ASSERT_NE(emitter, nullptr);
    ASSERT_NE(sprite, nullptr);
    ASSERT_NE(animator, nullptr);
    ASSERT_NE(collider, nullptr);

    EXPECT_TRUE(loaded.HasComponent<BillboardTagComponent>());

    TestUtils::GLM_VecExpectFloat(transform->transform.GetPosition(), glm::vec3{0.0f});
    TestUtils::GLM_VecExpectFloat(transform->transform.GetRotation(), glm::vec3{0.0f});
    TestUtils::GLM_VecExpectFloat(transform->transform.GetScale(), glm::vec3{1.0f});

    const MovementComponent movementDefaults;
    EXPECT_FLOAT_EQ(movement->timePerStep, movementDefaults.timePerStep);
    EXPECT_EQ(movement->autoMove, movementDefaults.autoMove);

    EXPECT_EQ(mesh->mesh, nullptr);
    EXPECT_EQ(material->material, nullptr);
    EXPECT_EQ(textures->index, 0);

    for (const auto &texture : textures->textures) {
        EXPECT_EQ(texture, nullptr);
    }

    const PointLightComponent lightDefaults;
    TestUtils::GLM_VecExpectFloat(light->color, lightDefaults.color);
    EXPECT_FLOAT_EQ(light->radius, lightDefaults.radius);
    EXPECT_FLOAT_EQ(light->intensity, lightDefaults.intensity);

    EXPECT_TRUE(script->slots.empty());
    EXPECT_EQ(sprite->sheet, nullptr);
    EXPECT_EQ(sprite->frame, 0u);
    EXPECT_EQ(animator->animations, nullptr);
    EXPECT_TRUE(animator->initialClip.empty());
    EXPECT_TRUE(animator->autoplay);
    EXPECT_FALSE(animator->playing);
    EXPECT_EQ(*collider, ColliderComponent{});

    auto defaultEntity = CreateEntity(sourceRegistry);
    defaultEntity.AddComponent<ParticleEmitterComponent>();

    EXPECT_EQ(Serialize(loaded).at("components").at("ParticleEmitterComponent"), Serialize(defaultEntity).at("components").at("ParticleEmitterComponent"));
}

TEST_F(EntitySerializationTests, UnknownComponentDoesNotPreventKnownComponentsFromLoading) {
    const nlohmann::json data{{"components", {{"FutureComponent", {{"unknownField", 42}}}, {"MovementComponent", {{"autoMove", true}}}, {"TransformComponent", {{"position", {1, 2, 3}}}}}}};

    ECS::Entity loaded;
    ASSERT_NO_THROW(loaded = Load(data));

    const auto *movement = loaded.GetComponent<ECS::Components::MovementComponent>();
    const auto *transform = loaded.GetComponent<ECS::Components::TransformComponent>();

    ASSERT_NE(movement, nullptr);
    ASSERT_NE(transform, nullptr);

    EXPECT_TRUE(movement->autoMove);

    TestUtils::GLM_VecExpectFloat(transform->transform.GetPosition(), glm::vec3{1, 2, 3});

    EXPECT_FALSE(Serialize(loaded).at("components").contains("FutureComponent"));
}

TEST_F(EntitySerializationTests, InvalidOrMissingUUIDKeepsGeneratedIdentity) {
    const std::vector<nlohmann::json> invalidUUIDs{nullptr, "", 12, false, nlohmann::json::array(), nlohmann::json::object()};

    for (const auto &uuid : invalidUUIDs) {
        SCOPED_TRACE(uuid.dump());

        auto loaded = CreateEntity(destinationRegistry);
        const auto original = GetUUID(loaded);

        const nlohmann::json data{{"uuid", uuid}, {"components", {{"MovementComponent", {{"autoMove", true}}}}}};

        ASSERT_NO_THROW(IO::EntityFactory::DeserializeEntity(loaded, data, resources, true));

        EXPECT_EQ(GetUUID(loaded), original);

        const auto *movement = loaded.GetComponent<ECS::Components::MovementComponent>();

        ASSERT_NE(movement, nullptr);
        EXPECT_TRUE(movement->autoMove);
    }

    auto loaded = CreateEntity(destinationRegistry);
    const auto original = GetUUID(loaded);

    ASSERT_NO_THROW(IO::EntityFactory::DeserializeEntity(loaded, nlohmann::json::object(), resources, true));

    EXPECT_EQ(GetUUID(loaded), original);
}

TEST_F(EntitySerializationTests, WrongFieldTypesDoNotPreventOtherComponentsFromLoading) {
    const std::vector<std::pair<std::string, nlohmann::json>> cases{{"TransformComponent", {{"position", {1, "bad", 3}}}}, {"MovementComponent", {{"timePerStep", "bad"}}}, {"MovementComponent", {{"autoMove", "bad"}}},
            {"MeshComponent", {{"mesh_id", 42}}}, {"MaterialComponent", {{"material_id", false}}}, {"DirectionalTextureComponent", {{"textures", {42}}}}, {"PointLightComponent", {{"radius", "bad"}}},
            {"ParticleEmitterComponent", {{"maxParticles", "bad"}}}, {"SpriteSheetComponent", {{"texture_id", 42}}}, {"SpriteAnimatorComponent", {{"autoplay", "bad"}}},
            {"ColliderComponent", {{"version", 2}, {"radius", "bad"}}}};

    for (const auto &[name, component] : cases) {
        SCOPED_TRACE(name + ": " + component.dump());

        ECS::Entity loaded;

        ASSERT_NO_THROW(loaded = Load({{"components", {{name, component}, {"BillboardTagComponent", nlohmann::json::object()}}}}));

        EXPECT_TRUE(loaded.HasComponent<ECS::Components::BillboardTagComponent>());

        const auto saved = Serialize(loaded);

        ASSERT_TRUE(saved.contains("components"));
        EXPECT_FALSE(saved.at("components").contains(name));
    }
}

TEST_F(EntitySerializationTests, ResourceReferencesKeepCatalogKeys) {
    auto mesh = std::make_shared<Rendering::Mesh>(Rendering::MeshData{});
    auto material = std::make_shared<Rendering::Material>();
    auto texture = CreateTexture();

    mesh->SetFactoryId("different-from-catalog-key");
    texture->GetPath() = "assets/different-from-catalog-key.png";

    const auto meshKey = RegisterAsset(mesh);
    const auto materialKey = RegisterAsset(material);
    const auto textureKey = RegisterAsset(texture);

    auto source = CreateEntity(sourceRegistry);

    source.AddComponent<ECS::Components::MeshComponent>().mesh = mesh;
    source.AddComponent<ECS::Components::MaterialComponent>().material = material;
    source.AddComponent<ECS::Components::ParticleEmitterComponent>().material = material;

    auto &directional = source.AddComponent<ECS::Components::DirectionalTextureComponent>();

    directional.textures = {texture, nullptr, texture, nullptr, nullptr, texture};
    directional.index = 5;

    const auto saved = Serialize(source);
    const auto &components = saved.at("components");

    EXPECT_EQ(components.at("MeshComponent").at("mesh_id"), meshKey);
    EXPECT_EQ(components.at("MaterialComponent").at("material_id"), materialKey);
    EXPECT_EQ(components.at("ParticleEmitterComponent").at("material_id"), materialKey);
    EXPECT_EQ(components.at("DirectionalTextureComponent").at("textures"), nlohmann::json::array({textureKey, "", textureKey, "", "", textureKey}));

    auto loaded = RoundTrip(source);

    const auto *actualMesh = loaded.GetComponent<ECS::Components::MeshComponent>();
    const auto *actualMaterial = loaded.GetComponent<ECS::Components::MaterialComponent>();
    const auto *actualEmitter = loaded.GetComponent<ECS::Components::ParticleEmitterComponent>();
    const auto *actualDirectional = loaded.GetComponent<ECS::Components::DirectionalTextureComponent>();

    ASSERT_NE(actualMesh, nullptr);
    ASSERT_NE(actualMaterial, nullptr);
    ASSERT_NE(actualEmitter, nullptr);
    ASSERT_NE(actualDirectional, nullptr);

    EXPECT_EQ(actualMesh->mesh, mesh);
    EXPECT_EQ(actualMaterial->material, material);
    EXPECT_EQ(actualEmitter->material, material);
    EXPECT_EQ(actualDirectional->textures, directional.textures);
    EXPECT_EQ(actualDirectional->index, directional.index);
}

TEST_F(EntitySerializationTests, StandaloneSpriteSheetRoundtrip) {
    auto texture = CreateTexture();
    texture->GetPath() = "assets/not-the-resource-key.png";

    const auto key = RegisterAsset(texture);
    auto animation = CreateAnimation(texture);

    auto source = CreateEntity(sourceRegistry);
    auto &sprite = source.AddComponent<ECS::Components::SpriteSheetComponent>();

    sprite.sheet = animation->sheet;
    sprite.frame = 6;

    EXPECT_EQ(Serialize(source).at("components").at("SpriteSheetComponent").at("texture_id"), key);

    auto loaded = RoundTrip(source);
    const auto *actual = loaded.GetComponent<ECS::Components::SpriteSheetComponent>();

    ASSERT_NE(actual, nullptr);
    ASSERT_NE(actual->sheet, nullptr);

    ExpectSheet(*actual->sheet, *sprite.sheet);
    EXPECT_EQ(actual->frame, sprite.frame);
}

TEST_F(EntitySerializationTests, SpriteSheetMissingOptionalFieldsUseDefaults) {
    auto texture = CreateTexture();
    const auto key = RegisterAsset(texture);

    auto loaded = Load({{"components", {{"SpriteSheetComponent", {{"texture_id", key}}}}}});

    const auto *actual = loaded.GetComponent<ECS::Components::SpriteSheetComponent>();

    ASSERT_NE(actual, nullptr);
    ASSERT_NE(actual->sheet, nullptr);

    Rendering::SpriteSheet expected;
    expected.texture = texture;

    ExpectSheet(*actual->sheet, expected);
    EXPECT_EQ(actual->frame, 0u);
}

TEST_F(EntitySerializationTests, AnimatorRoundtripRestartsInitialClip) {
    auto texture = CreateTexture();
    RegisterAsset(texture);

    auto animation = CreateAnimation(texture);
    const auto key = RegisterAsset(animation);

    for (const bool autoplay : {false, true}) {
        SCOPED_TRACE(autoplay);

        auto source = CreateEntity(sourceRegistry);
        auto &player = source.AddComponent<ECS::Components::SpriteAnimatorComponent>();

        player.animations = animation;
        player.initialClip = "idle";
        player.autoplay = autoplay;

        // Runtime state should not become the saved initial state.
        player.clip = "once";
        player.frameIndex = 1;
        player.elapsed = 0.125;
        player.playing = !autoplay;

        auto &sprite = source.AddComponent<ECS::Components::SpriteSheetComponent>();

        sprite.sheet = animation->sheet;
        sprite.frame = 4;

        const auto saved = Serialize(source);

        EXPECT_EQ(saved.at("components").at("SpriteAnimatorComponent"), nlohmann::json({{"animation_id", key}, {"initial_clip", "idle"}, {"autoplay", autoplay}}));

        // The animator supplies its sheet when loaded.
        EXPECT_EQ(saved.at("components").at("SpriteSheetComponent"), nlohmann::json::object());

        auto loaded = RoundTrip(source);

        const auto *actual = loaded.GetComponent<ECS::Components::SpriteAnimatorComponent>();
        const auto *actualSprite = loaded.GetComponent<ECS::Components::SpriteSheetComponent>();

        ASSERT_NE(actual, nullptr);
        ASSERT_NE(actualSprite, nullptr);

        EXPECT_EQ(actual->animations, animation);
        EXPECT_EQ(actual->initialClip, "idle");
        EXPECT_EQ(actual->autoplay, autoplay);
        EXPECT_EQ(actual->clip, "idle");
        EXPECT_EQ(actual->frameIndex, 0u);
        EXPECT_DOUBLE_EQ(actual->elapsed, 0.0);
        EXPECT_EQ(actual->playing, autoplay);

        EXPECT_EQ(actualSprite->sheet, animation->sheet);
        EXPECT_EQ(actualSprite->frame, 2u);
    }
}

TEST_F(EntitySerializationTests, AnimatorCreatesMissingSpriteComponent) {
    auto animation = CreateAnimation(CreateTexture());
    const auto key = RegisterAsset(animation);

    auto loaded = Load({{"components", {{"SpriteAnimatorComponent", {{"animation_id", key}, {"initial_clip", "idle"}}}}}});

    const auto *player = loaded.GetComponent<ECS::Components::SpriteAnimatorComponent>();
    const auto *sprite = loaded.GetComponent<ECS::Components::SpriteSheetComponent>();

    ASSERT_NE(player, nullptr);
    ASSERT_NE(sprite, nullptr);

    EXPECT_TRUE(player->autoplay);
    EXPECT_TRUE(player->playing);
    EXPECT_EQ(sprite->sheet, animation->sheet);
    EXPECT_EQ(sprite->frame, 2u);
}

TEST_F(EntitySerializationTests, MissingAnimationOrClipLeavesSafeEmptyPose) {
    auto animation = CreateAnimation(CreateTexture());
    const auto key = RegisterAsset(animation);

    const auto missingKey = "missing-animation-" + Core::Utils::UUID::UUIDGenerator::Generate();

    const std::vector<nlohmann::json> cases{nlohmann::json::object(), {{"animation_id", missingKey}, {"initial_clip", "idle"}}, {{"animation_id", key}, {"initial_clip", "missing-clip"}}, {{"animation_id", key}}};

    for (const auto &component : cases) {
        SCOPED_TRACE(component.dump());

        ECS::Entity loaded;

        ASSERT_NO_THROW(loaded = Load({{"components", {{"SpriteAnimatorComponent", component}}}}));

        const auto *player = loaded.GetComponent<ECS::Components::SpriteAnimatorComponent>();
        const auto *sprite = loaded.GetComponent<ECS::Components::SpriteSheetComponent>();

        ASSERT_NE(player, nullptr);
        ASSERT_NE(sprite, nullptr);

        EXPECT_TRUE(player->clip.empty());
        EXPECT_FALSE(player->playing);
        EXPECT_EQ(player->frameIndex, 0u);
        EXPECT_DOUBLE_EQ(player->elapsed, 0.0);

        EXPECT_EQ(sprite->sheet, nullptr);
        EXPECT_EQ(sprite->frame, 0u);
    }
}

TEST_F(EntitySerializationTests, AnimationAssetRoundtripPreservesSheetClipsAndDurations) {
    auto texture = CreateTexture();
    texture->GetPath() = "assets/not-the-texture-key.png";

    const auto key = RegisterAsset(texture);
    auto animation = CreateAnimation(texture);

    const auto saved = IO::AnimationIO::AnimationToJson(*animation);

    ASSERT_FALSE(saved.is_null());
    EXPECT_EQ(saved.at("sheet").at("texture_id"), key);

    const auto loaded = IO::AnimationIO::JsonToAnimation(nlohmann::json::parse(saved.dump()));

    ASSERT_NE(loaded.sheet, nullptr);
    ExpectSheet(*loaded.sheet, *animation->sheet);

    ASSERT_EQ(loaded.clips.size(), animation->clips.size());

    for (const auto &[name, expected] : animation->clips) {
        SCOPED_TRACE(name);

        const auto it = loaded.clips.find(name);
        ASSERT_NE(it, loaded.clips.end());

        const auto &actual = it->second;

        EXPECT_EQ(actual.loop, expected.loop);
        ASSERT_EQ(actual.frames.size(), expected.frames.size());

        for (std::size_t i = 0; i < expected.frames.size(); ++i) {
            SCOPED_TRACE(i);

            EXPECT_EQ(actual.frames[i].index, expected.frames[i].index);
            EXPECT_FLOAT_EQ(actual.frames[i].duration, expected.frames[i].duration);
        }
    }

    EXPECT_TRUE(Animation::ValidateSet(loaded));
}

TEST_F(EntitySerializationTests, AnimationAssetMissingOptionalFieldsUseDefaults) {
    auto texture = CreateTexture();
    const auto key = RegisterAsset(texture);

    const auto loaded = IO::AnimationIO::JsonToAnimation({{"sheet", {{"texture_id", key}}}, {"clips", {{"idle", {{"frames", nlohmann::json::array({nlohmann::json::object()})}}}}}});

    ASSERT_NE(loaded.sheet, nullptr);

    Rendering::SpriteSheet expected;
    expected.texture = texture;

    ExpectSheet(*loaded.sheet, expected);

    const auto it = loaded.clips.find("idle");
    ASSERT_NE(it, loaded.clips.end());

    EXPECT_TRUE(it->second.loop);
    ASSERT_EQ(it->second.frames.size(), 1u);
    EXPECT_EQ(it->second.frames[0].index, 0u);
    EXPECT_FLOAT_EQ(it->second.frames[0].duration, 0.1f);
}
