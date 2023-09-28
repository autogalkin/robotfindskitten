#pragma once
#include "engine/details/base_types.h"
#include "engine/ecs_processor_base.h"

class killer {
  public:
    void execute(entt::registry& reg, timings::duration delta) override {

        for (const auto view = reg.view<const life::begin_die>();
             const auto entity : view) {
            reg.destroy(entity);
        }
    }
};
