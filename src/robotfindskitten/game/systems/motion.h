#pragma once
#ifndef _CPP_PROJECTS_ROBOTFINDSKITTEN_SRC_ROBOTFINDSKITTEN_GAME_ECS_PROCESSORS_MOTION_H
#define _CPP_PROJECTS_ROBOTFINDSKITTEN_SRC_ROBOTFINDSKITTEN_GAME_ECS_PROCESSORS_MOTION_H

#include "engine/details/base_types.hpp"
#include "engine/time.h"
#include "engine/world.h"

struct uniform_movement_tag {};
struct non_uniform_movement_tag {};

struct velocity {
    loc v{0};
};
using translation = dirty_flag<loc>;

struct non_uniform_motion: is_system {
    inline static constexpr velocity FRICTION_FACTOR{0.7, 0.7};
    void operator()(
        entt::view<
            entt::get_t<translation, velocity, const non_uniform_movement_tag>>
            view) {
        view.each([](auto ent, auto& trans, auto& vel) {
            using namespace std::literals;
            // double alpha = std::chrono::duration<double>(dt / 1s).count();
            double alpha = 1. / 60.; // NOLINT(readability-magic-numbers)
            loc friction = -(vel * FRICTION_FACTOR);
            vel.v += friction * loc(alpha);
            // alpha = lag accum / dt
            // oldvel = vel;
            // vel += friction * dt/1s
            // (vel)* alpha + oldvel * (1-alpha)
            trans.pin() = vel.v * static_cast<double>(alpha);
        });
    }
};

struct uniform_motion: is_system {
    void
    operator()(entt::view<
               entt::get_t<translation, velocity, const uniform_movement_tag>>
                   view) {
        view.each([](auto ent, auto& trans, auto& vel) {
            if(vel.v != loc(0)) {
                trans.pin() = std::exchange(vel, vel{});
            }
        });
    }
};

#endif
