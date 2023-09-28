#pragma once
#include "engine/details/base_types.h"
#include "engine/notepad.h"
#include "engine/world.h"
#include "engine/time.h"

struct non_uniform_motion{
    inline static const velocity friction_factor{0.7f, 0.7f};

    void execute(entt::registry& reg, timings::duration delta);
};

struct uniform_motion  {
    void execute(entt::registry& reg, timings::duration delta);
};
