/**
 * @file
 * @brief The systems for moving actors in the game
 */

#ifndef _CPP_PROJECTS_ROBOTFINDSKITTEN_SRC_ROBOTFINDSKITTEN_GAME_ECS_PROCESSORS_MOTION_H
#define _CPP_PROJECTS_ROBOTFINDSKITTEN_SRC_ROBOTFINDSKITTEN_GAME_ECS_PROCESSORS_MOTION_H

#include <entt/entity/registry.hpp>

#include "engine/details/base_types.hpp"
#include "engine/time.h"

/**
 * @brief A tag for movement without pseudo physics.
 */
struct uniform_movement_tag {};
/**
 * @brief A tag for movement with a pseudo friction.
 */
struct non_uniform_movement_tag {};

/**
 * @brief Acs as a force. Applies to the translation in the every tick.
 *
 * Inside the uniform_motion system, the velocity resets to 0 every tick, and
 * the entity should set a new velocity afterward
 * Within the non_uniform_motion system, the velocity slowly decreases to 0 due
 * to pseudo-friction
 */
struct velocity {
    loc v{0};
};

/**
 * @brief Accumullates all movement from all systems and then applies to the
 * entity's location in the \ref drawing::redrawer after erasing a previous
 * entity sprite position from the game buffer
 */
using translation = dirty_flag<loc>;

namespace motion {

struct non_uniform_motion {
    inline static constexpr velocity FRICTION_FACTOR{.v = {0.7, 0.7}};
    void operator()(
        const entt::view<entt::get_t<translation, velocity,
                                     const non_uniform_movement_tag>>& view,
        timings::duration dt) {
        view.each([&dt](auto, auto& trans, auto& vel) {
            using namespace std::literals;
            double alpha = std::chrono::duration<double>(dt).count();
            loc friction = -(vel.v * FRICTION_FACTOR.v);
            vel.v += friction * loc(alpha);
            trans.pin() = vel.v * static_cast<double>(alpha);
        });
    }
};

struct uniform_motion {
    explicit uniform_motion() = default;
    void operator()(
        const entt::view<entt::get_t<translation, velocity,
                                     const uniform_movement_tag>>& view) {
        view.each([](auto, auto& trans, auto& vel) {
            if(vel.v != loc(0)) {
                trans.pin() = std::exchange(vel, velocity{}).v;
            }
        });
    }
};
} // namespace motion

#endif
