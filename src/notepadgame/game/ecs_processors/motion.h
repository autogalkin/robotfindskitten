#pragma once
#include "engine/details/base_types.h"
#include "engine/notepad.h"
#include "engine/world.h"
#include "engine/time.h"


struct uniform_movement_tag {};
struct non_uniform_movement_tag {};

struct velocity    : loc{
    velocity(double x, double y): loc(x, y){}
    velocity(): loc(0){}
};
using translation  =dirty_flag<loc>;


struct non_uniform_motion{
    inline static const velocity friction_factor{0.7, 0.7};

    void execute(entt::registry& reg, timings::duration delta);
};

struct uniform_motion  {
    void execute(entt::registry& reg, timings::duration delta);
};
