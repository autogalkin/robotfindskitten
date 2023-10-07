#include "engine/details/base_types.hpp"
#include "game/ecs_processors/collision.h"
#include <gtest/gtest.h>
#include <entt/entt.hpp>
// NOLINTBEGIN(readability-magic-numbers)


TEST(CollisionTest, InsertItem) {
    using namespace collision;
    auto query = quad_tree{box{{0, 0}, {10, 10}}};
    box b {{0, 0},{3, 3}};
    // TODO(Igor): own type for id
    auto e = static_cast<entt::entity>(10);
    auto index = query.insert(e, b);
    auto res = query.get_entity(index);
    EXPECT_EQ(res.id, e);
    EXPECT_EQ(res.bbox, b);
}

TEST(CollisionTest, RemoveItem) {
    using namespace collision;
    auto query = quad_tree{box{{0, 0}, {10, 10}}};
    box b {{0, 0},{3, 3}};
    // TODO(Igor): own type for id
    auto e = static_cast<entt::entity>(10);
    auto index = query.insert(e, b);
    //auto res = query.get_entity(index);
    // TODO(Igor): res return
    query.remove(index);
    // TODO(Igor): size api
    // TODO(API): has entity
    query.get_entity(index);
}

// NOLINTEND(readability-magic-numbers)
