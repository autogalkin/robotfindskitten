#pragma once
#include "details/base_types.h"
#include "ecs_processor_base.h"
#include "notepad.h"
#include "world.h"

class non_uniform_motion final : public ecs_processor {
  public:
    inline static const velocity friction_factor{0.7f, 0.7f};

    explicit non_uniform_motion(world* w) : ecs_processor(w) {}
    void execute(entt::registry& reg, gametime::duration delta) override;
};

class uniform_motion final : public ecs_processor {
  public:
    explicit uniform_motion(world* w) : ecs_processor(w) {}
    void execute(entt::registry& reg, gametime::duration delta) override;
    ;
};
