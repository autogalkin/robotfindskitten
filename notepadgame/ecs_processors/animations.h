#pragma once

#include <variant>

#include "../core/base_types.h"
#include "../core/easing.h"

struct easing_data{
    double(*ease_func)(double);
    double* data_ptr;
};

class easing_processor final : public ecs_processor
{
public:
    explicit easing_processor(world* w)
        : ecs_processor(w)
    {
    }

    void execute(entt::registry& reg, gametime::duration delta) override
    {
        for(const auto view = reg.view<shape::sprite, life::lifetime, easing_data>();
        const auto entity: view)
        {
            
            auto& lt = view.get<life::lifetime>(entity);

            const auto& [ease_func, data_ptr] = view.get<easing_data>(entity);
            *data_ptr = easing::easeinrange(*data_ptr, {0.,1.}, 100, ease_func);
            
            
        }
    }
};

