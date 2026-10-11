#include "Scripting/EngineLib/EntityWrapperCache.h"
#include "ECS/Components/RelationshipComponent.h"
#include <gtest/gtest.h>
#include <unordered_set>

using Scripting::EntityWrapperCache;

namespace {

    class EntityWrapperCacheTests : public ::testing::Test {
    protected:
        void TearDown() override { EntityWrapperCache::ClearAll(); }

        ObSL::Interpreter m_Interp{"/nonexistent-script-root"};

        ECS::Registry m_Registry;

        EntityWrapperCache::Cache *Cache() {
            EntityWrapperCache::RegisterInterpreter(&m_Interp, nullptr);
            return EntityWrapperCache::Get(&m_Interp);
        }
    };

    TEST_F(EntityWrapperCacheTests, StoredWrapperResolvesUntilEvicted) {
        const auto id = m_Registry.CreateEntity();
        auto *cache = Cache();
        auto *wrapper = m_Interp.gc.allocate<ObSL::ObSLObject>();

        cache->Store(m_Registry, id, EntityWrapperCache::Kind::Entity, wrapper);

        const auto found = cache->Find(m_Registry, id, EntityWrapperCache::Kind::Entity, [] { return true; });
        ASSERT_EQ(found, wrapper);

        EntityWrapperCache::EvictAll(id);

        EXPECT_EQ(cache->Find(m_Registry, id, EntityWrapperCache::Kind::Entity, [] { return true; }), nullptr);
    }

    TEST_F(EntityWrapperCacheTests, EvictRemovesEveryKindOfWrapperForTheEntity) {
        const auto id = m_Registry.CreateEntity();
        auto *cache = Cache();
        for (const auto kind : {EntityWrapperCache::Kind::Entity, EntityWrapperCache::Kind::Transform, EntityWrapperCache::Kind::Movement}) {
            cache->Store(m_Registry, id, kind, m_Interp.gc.allocate<ObSL::ObSLObject>());
        }

        cache->Evict(id);

        for (const auto kind : {EntityWrapperCache::Kind::Entity, EntityWrapperCache::Kind::Transform, EntityWrapperCache::Kind::Movement}) {
            EXPECT_EQ(cache->Find(m_Registry, id, kind, [] { return true; }), nullptr);
        }
    }

    TEST_F(EntityWrapperCacheTests, EvictLeavesOtherEntitiesUntouched) {
        const auto first = m_Registry.CreateEntity();
        const auto second = m_Registry.CreateEntity();
        auto *cache = Cache();
        auto *wrapperFirst = m_Interp.gc.allocate<ObSL::ObSLObject>();
        auto *wrapperSecond = m_Interp.gc.allocate<ObSL::ObSLObject>();
        cache->Store(m_Registry, first, EntityWrapperCache::Kind::Entity, wrapperFirst);
        cache->Store(m_Registry, second, EntityWrapperCache::Kind::Entity, wrapperSecond);

        cache->Evict(first);

        EXPECT_EQ(cache->Find(m_Registry, first, EntityWrapperCache::Kind::Entity, [] { return true; }), nullptr);
        EXPECT_EQ(cache->Find(m_Registry, second, EntityWrapperCache::Kind::Entity, [] { return true; }), wrapperSecond);
    }

    TEST_F(EntityWrapperCacheTests, EvictUnknownEntityIsANoOp) {
        auto *cache = Cache();
        EXPECT_NO_FATAL_FAILURE(cache->Evict(123456));
        EXPECT_NO_FATAL_FAILURE(EntityWrapperCache::EvictAll(654321));
    }

    TEST_F(EntityWrapperCacheTests, RegistryDestructionCallbackFiresForEntityAndDescendants) {
        ECS::Registry registry;
        const auto parent = registry.CreateEntity();
        const auto child = registry.CreateEntity();

        registry.AddComponent<ECS::Components::RelationshipComponent>(parent);
        registry.AddComponent<ECS::Components::RelationshipComponent>(child);
        registry.Reparent(child, parent);
        {
            const auto *parentRel = registry.GetComponent<ECS::Components::RelationshipComponent>(parent);
            ASSERT_NE(parentRel, nullptr);
            ASSERT_EQ(parentRel->children.size(), 1u);
        }

        std::unordered_set<ECS::EntityID> destroyed;
        ECS::Registry::SetEntityDestroyedCallback([&destroyed](const ECS::EntityID id) { destroyed.insert(id); });

        registry.RequestDestroy(parent);
        registry.DestroyEntity(parent);

        ECS::Registry::SetEntityDestroyedCallback(nullptr);

        EXPECT_EQ(destroyed.size(), 2u);
        EXPECT_TRUE(destroyed.contains(parent));
        EXPECT_TRUE(destroyed.contains(child));
        EXPECT_FALSE(registry.IsValid(parent));
        EXPECT_FALSE(registry.IsValid(child));
    }

} // namespace
