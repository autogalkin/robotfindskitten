#pragma once
#include "engine/details/base_types.h"
#include "engine/ecs_processor_base.h"
#include "engine/notepad.h"
#include "engine/world.h"
#include "engine/time.h"

class non_uniform_motion{
  public:
    inline static const velocity friction_factor{0.7f, 0.7f};

    void execute(entt::registry& reg, timings::duration delta) override;
};

class uniform_motion  {
  public:
    void execute(entt::registry& reg, timings::duration delta) override;
};
