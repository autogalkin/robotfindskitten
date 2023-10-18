
#ifndef _CPP_PROJECTS_ROBOTFINDSKITTEN_SRC_ROBOTFINDSKITTEN_GAME_ECS_PROCESSORS_MOTION_H
#define _CPP_PROJECTS_ROBOTFINDSKITTEN_SRC_ROBOTFINDSKITTEN_GAME_ECS_PROCESSORS_MOTION_H

#include <entt/entity/entity.hpp>
#include <entt/entt.hpp>

#include "engine/details/base_types.hpp"
#include "engine/time.h"

struct uniform_movement_tag {};
struct non_uniform_movement_tag {};

struct velocity {
    loc v{0};
};
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
