#include <gtest/gtest.h>
#include "ECS/Registry.h"
#include <algorithm>
#include <unordered_set>
#include <utility>
#include <vector>

namespace {
    using Relationship = ECS::Components::RelationshipComponent;

    TEST(HierarchySafetyTests, DirectParentingRejectsTwoEntityCycle) {
        ECS::Registry registry;
        const auto a = registry.CreateEntity();
        const auto b = registry.CreateEntity();
        registry.SetParentDirect(a, b);
        registry.SetParentDirect(b, a);
        ASSERT_EQ(registry.GetComponent<Relationship>(b)->parent, ECS::INVALID_ENTITY_ID);
        ASSERT_EQ(registry.GetComponent<Relationship>(a)->parent, b);
        registry.DestroyEntity(b);
        EXPECT_FALSE(registry.IsValid(a));
        EXPECT_FALSE(registry.IsValid(b));
    }

    TEST(HierarchySafetyTests, RejectionPreservesOldParentForBothSetters) {
        for (const bool direct : {false, true}) {
            ECS::Registry registry;
            const auto oldParent = registry.CreateEntity();
            const auto child = registry.CreateEntity();
            const auto descendant = registry.CreateEntity();
            registry.SetParentDirect(child, oldParent);
            registry.SetParentDirect(descendant, child);
            if (direct) {
                registry.SetParentDirect(child, descendant);
            } else {
                registry.Reparent(child, descendant);
            }
            EXPECT_EQ(registry.GetComponent<Relationship>(child)->parent, oldParent);
            const auto &children = registry.GetComponent<Relationship>(oldParent)->children;
            EXPECT_EQ(std::count(children.begin(), children.end(), child), 1);
            EXPECT_TRUE(registry.GetComponent<Relationship>(descendant)->children.empty());
        }
    }

    TEST(HierarchySafetyTests, ExistingAncestorCycleIsRejectedWithoutHanging) {
        ECS::Registry registry;
        const auto a = registry.CreateEntity();
        const auto b = registry.CreateEntity();
        const auto child = registry.CreateEntity();
        registry.AddComponent<Relationship>(a).parent = b;
        registry.AddComponent<Relationship>(b).parent = a;
        registry.AddComponent<Relationship>(child);
        EXPECT_FALSE(registry.CanSetParent(child, a));
        registry.SetParentDirect(child, a);
        registry.Reparent(child, a);
        EXPECT_EQ(registry.GetComponent<Relationship>(child)->parent, ECS::INVALID_ENTITY_ID);
        registry.SetParentDirect(a, ECS::INVALID_ENTITY_ID);
        EXPECT_EQ(registry.GetComponent<Relationship>(a)->parent, ECS::INVALID_ENTITY_ID);
    }

    TEST(HierarchySafetyTests, StaleParentDoesNotDetachChild) {
        for (const bool direct : {false, true}) {
            ECS::Registry registry;
            const auto parent = registry.CreateEntity();
            const auto child = registry.CreateEntity();
            const auto stale = registry.CreateEntity();
            registry.SetParentDirect(child, parent);
            registry.DestroyEntity(stale);
            if (direct) {
                registry.SetParentDirect(child, stale);
            } else {
                registry.Reparent(child, stale);
            }
            EXPECT_EQ(registry.GetComponent<Relationship>(child)->parent, parent);
            EXPECT_FALSE(registry.CanSetParent(stale, parent));
            EXPECT_FALSE(registry.CanSetParent(child, child));
            EXPECT_TRUE(registry.CanSetParent(child, ECS::INVALID_ENTITY_ID));
        }
    }

    TEST(HierarchySafetyTests, RepeatedDirectParentingDoesNotDuplicate) {
        ECS::Registry registry;
        const auto parent = registry.CreateEntity();
        const auto child = registry.CreateEntity();
        for (int i = 0; i < 3; ++i) {
            registry.SetParentDirect(child, parent);
        }
        const auto &children = registry.GetComponent<Relationship>(parent)->children;
        EXPECT_EQ(std::count(children.begin(), children.end(), child), 1);
        registry.SetParentDirect(child, ECS::INVALID_ENTITY_ID);
        EXPECT_EQ(registry.GetComponent<Relationship>(child)->parent, ECS::INVALID_ENTITY_ID);
        EXPECT_TRUE(registry.GetComponent<Relationship>(parent)->children.empty());
    }
} // namespace

namespace {

    struct CleanupProbe {
        ECS::EntityID id;
        std::vector<ECS::EntityID> *order;

        CleanupProbe(ECS::EntityID entity, std::vector<ECS::EntityID> *output) : id(entity), order(output) {}
        CleanupProbe(const CleanupProbe &) = delete;
        CleanupProbe &operator=(const CleanupProbe &) = delete;
        CleanupProbe(CleanupProbe &&other) noexcept : id(other.id), order(std::exchange(other.order, nullptr)) {}
        CleanupProbe &operator=(CleanupProbe &&other) noexcept {
            if (this != &other) {
                if (order) {
                    order->push_back(id);
                }
                id = other.id;
                order = std::exchange(other.order, nullptr);
            }
            return *this;
        }
        ~CleanupProbe() {
            if (order) {
                order->push_back(id);
            }
        }
    };

    TEST(HierarchySafetyTests, DeepTreePreservesUnrelatedEntity) {
        ECS::Registry registry;
        std::vector<ECS::EntityID> chain;
        for (uint32_t i = 0; i < ECS::MAX_ENTITIES - 2; ++i) {
            const auto id = registry.CreateEntity();
            registry.AddComponent<Relationship>(id);
            chain.push_back(id);
        }
        const auto unrelated = registry.CreateEntity();
        for (size_t i = 1; i < chain.size(); ++i) {
            registry.GetComponent<Relationship>(chain[i])->parent = chain[i - 1];
            registry.GetComponent<Relationship>(chain[i - 1])->children.push_back(chain[i]);
        }
        registry.DestroyEntity(chain.front());
        for (const auto id : chain) {
            EXPECT_FALSE(registry.IsValid(id));
            EXPECT_FALSE(registry.GetPool<Relationship>()->Has(id));
        }
        EXPECT_EQ(registry.GetLivingEntities(), std::vector<ECS::EntityID>{unrelated});
        std::unordered_set<uint32_t> recycled;
        for (size_t i = 0; i < chain.size(); ++i) {
            const auto id = registry.CreateEntity();
            EXPECT_EQ(ECS::GetEntityVersion(id), 1u);
            EXPECT_TRUE(recycled.insert(ECS::GetEntityIndex(id)).second);
        }
        EXPECT_TRUE(registry.IsValid(unrelated));
        EXPECT_EQ(registry.GetLivingEntities().size(), ECS::MAX_ENTITIES - 1);
    }

    TEST(HierarchySafetyTests, SelfReferenceIsDeletedOnce) {
        ECS::Registry registry;
        const auto id = registry.CreateEntity();
        const auto survivor = registry.CreateEntity();
        auto &rel = registry.AddComponent<Relationship>(id);
        rel.parent = id;
        rel.children = {id, id};
        registry.DestroyEntity(id);
        registry.DestroyEntity(id);
        EXPECT_FALSE(registry.IsValid(id));
        EXPECT_EQ(registry.GetLivingEntities(), std::vector<ECS::EntityID>{survivor});
    }

    TEST(HierarchySafetyTests, CycleAndDuplicateEdgesDeleteOnce) {
        ECS::Registry registry;
        const auto a = registry.CreateEntity();
        const auto b = registry.CreateEntity();
        const auto c = registry.CreateEntity();
        const auto survivor = registry.CreateEntity();
        for (const auto id : {a, b, c}) {
            registry.AddComponent<Relationship>(id);
        }
        registry.GetComponent<Relationship>(a)->parent = c;
        registry.GetComponent<Relationship>(a)->children = {b, b};
        registry.GetComponent<Relationship>(b)->parent = a;
        registry.GetComponent<Relationship>(b)->children = {c};
        registry.GetComponent<Relationship>(c)->parent = b;
        registry.GetComponent<Relationship>(c)->children = {a, ECS::INVALID_ENTITY_ID};
        while (registry.GetLivingEntities().size() < ECS::MAX_ENTITIES - 1) {
            registry.CreateEntity();
        }
        registry.DestroyEntity(a);
        EXPECT_FALSE(registry.IsValid(a));
        EXPECT_FALSE(registry.IsValid(b));
        EXPECT_FALSE(registry.IsValid(c));
        EXPECT_TRUE(registry.IsValid(survivor));
        std::unordered_set<uint32_t> recycled;
        for (int i = 0; i < 3; ++i) {
            const auto id = registry.CreateEntity();
            EXPECT_TRUE(recycled.insert(ECS::GetEntityIndex(id)).second);
            EXPECT_EQ(ECS::GetEntityVersion(id), 1u);
        }
        EXPECT_EQ(registry.GetLivingEntities().size(), ECS::MAX_ENTITIES - 1);
    }

    TEST(HierarchySafetyTests, SharedDescendantIsDeletedOnce) {
        ECS::Registry registry;
        const auto root = registry.CreateEntity();
        const auto a = registry.CreateEntity();
        const auto b = registry.CreateEntity();
        const auto shared = registry.CreateEntity();
        for (const auto id : {root, a, b, shared}) {
            registry.AddComponent<Relationship>(id);
        }
        registry.GetComponent<Relationship>(root)->children = {a, b};
        registry.GetComponent<Relationship>(a)->children = {shared};
        registry.GetComponent<Relationship>(b)->children = {shared};
        registry.DestroyEntity(root);
        EXPECT_TRUE(registry.GetLivingEntities().empty());
    }

    TEST(HierarchySafetyTests, StaleChildCannotDeleteReplacement) {
        ECS::Registry registry;
        const auto root = registry.CreateEntity();
        const auto stale = registry.CreateEntity();
        registry.AddComponent<Relationship>(root).children = {stale};
        while (registry.GetLivingEntities().size() < ECS::MAX_ENTITIES - 1) {
            registry.CreateEntity();
        }
        registry.DestroyEntity(stale);
        const auto replacement = registry.CreateEntity();
        ASSERT_EQ(ECS::GetEntityIndex(stale), ECS::GetEntityIndex(replacement));
        registry.DestroyEntity(root);
        EXPECT_TRUE(registry.IsValid(replacement));
        EXPECT_FALSE(registry.IsValid(root));
        EXPECT_EQ(registry.GetLivingEntities().size(), ECS::MAX_ENTITIES - 2);
    }

    TEST(HierarchySafetyTests, ComponentsAreCleanedInChildFirstOrder) {
        std::vector<ECS::EntityID> order;
        ECS::Registry registry;
        const auto root = registry.CreateEntity();
        const auto first = registry.CreateEntity();
        const auto grandchild = registry.CreateEntity();
        const auto second = registry.CreateEntity();
        registry.SetParentDirect(first, root);
        registry.SetParentDirect(grandchild, first);
        registry.SetParentDirect(second, root);
        for (const auto id : {root, first, grandchild, second}) {
            registry.AddComponent<CleanupProbe>(id, id, &order);
        }
        registry.DestroyEntity(root);
        EXPECT_EQ(order, (std::vector<ECS::EntityID>{grandchild, first, second, root}));
    }
} // namespace
