#pragma once
#ifndef _CPP_PROJECTS_ROBOTFINDSKITTEN_SRC_ROBOTFINDSKITTEN_GAME_ECS_PROCESSORS_MOTION_H
#define _CPP_PROJECTS_ROBOTFINDSKITTEN_SRC_ROBOTFINDSKITTEN_GAME_ECS_PROCESSORS_MOTION_H

#include <entt/entity/entity.hpp>
#include <entt/entt.hpp>

#include "engine/details/base_types.hpp"

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
        entt::view<
            entt::get_t<translation, velocity, const non_uniform_movement_tag>>
            // NOLINTNEXTLINE(performance-unnecessary-value-param)
            view) {
        view.each([](auto, auto& trans, auto& vel) {
            using namespace std::literals;
            // double alpha = std::chrono::duration<double>(dt / 1s).count();
            double alpha = 1. / 60.; // NOLINT(readability-magic-numbers)
            loc friction = -(vel.v * FRICTION_FACTOR.v);
            vel.v += friction * loc(alpha);
            // alpha = lag accum / dt
            // oldvel = vel;
            // vel += friction * dt/1s
            // (vel)* alpha + oldvel * (1-alpha)
            trans.pin() = vel.v * static_cast<double>(alpha);
        });
    }
};

struct uniform_motion {
    explicit uniform_motion() = default;
    void
    operator()(entt::view<
               entt::get_t<translation, velocity, const uniform_movement_tag>>
                   // NOLINTNEXTLINE(performance-unnecessary-value-param)
                   view) {
        view.each([](auto, auto& trans, auto& vel) {
            if(vel.v != loc(0)) {
                trans.pin() = std::exchange(vel, velocity{}).v;
            }
        });
    }
};
} // namespace motion

#endif
