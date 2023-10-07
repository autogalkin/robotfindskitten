#pragma once
#include "engine/details/base_types.hpp"
#include "engine/time.h"
#include "engine/world.h"


struct uniform_movement_tag {};
struct non_uniform_movement_tag {};

struct velocity: loc {
    velocity(double x, double y): loc(x, y) {}
    velocity(): loc(0) {}
};
using translation = dirty_flag<loc>;

struct non_uniform_motion: ecs_proc_tag {
    inline static const velocity friction_factor{0.7, 0.7};

    void execute(entt::registry& reg, timings::duration dt);
};

struct uniform_motion: ecs_proc_tag {
    void execute(entt::registry& reg, timings::duration dt);
};
