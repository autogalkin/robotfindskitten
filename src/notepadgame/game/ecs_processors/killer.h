#pragma once
#include "engine/details/base_types.h"

struct killer {
    void execute(entt::registry& reg, timings::duration delta) {

        for (const auto view = reg.view<const life::begin_die>();
             const auto entity : view) {
            reg.destroy(entity);
        }
    }
};
