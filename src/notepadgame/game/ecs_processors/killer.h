#pragma once
#include "details/base_types.h"
#include "ecs_processor_base.h"

class killer final : public ecs_processor {
  public:
    explicit killer(world* w) : ecs_processor(w) {}

    void execute(entt::registry& reg, time::duration delta) override {

        for (const auto view = reg.view<const life::begin_die>();
             const auto entity : view) {
            reg.destroy(entity);
        }
    }
};
