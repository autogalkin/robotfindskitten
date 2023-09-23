#pragma once
#include "engine/details/base_types.h"
#include "engine/ecs_processor_base.h"

class killer final : public ecs_processor {
  public:
    explicit killer(world* w) : ecs_processor(w) {}

    void execute(entt::registry& reg, time2::duration delta) override {

        for (const auto view = reg.view<const life::begin_die>();
             const auto entity : view) {
            reg.destroy(entity);
        }
    }
};
