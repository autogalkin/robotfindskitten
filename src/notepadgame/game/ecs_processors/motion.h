#pragma once
#include "engine/details/base_types.h"
#include "engine/ecs_processor_base.h"
#include "engine/notepad.h"
#include "engine/world.h"
#include "engine/timer.h"

class non_uniform_motion final : public ecs_processor {
  public:
    inline static const velocity friction_factor{0.7f, 0.7f};

    explicit non_uniform_motion(world* w) : ecs_processor(w) {}
    void execute(entt::registry& reg, time2::duration delta) override;
};

class uniform_motion final : public ecs_processor {
  public:
    explicit uniform_motion(world* w) : ecs_processor(w) {}
    void execute(entt::registry& reg, time2::duration delta) override;
    ;
};
