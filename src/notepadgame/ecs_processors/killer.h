#pragma once
#include "../core/base_types.h"


class killer final : public ecs_processor
{
public:
    explicit killer(world* w)
        : ecs_processor(w){
    }

    void execute(entt::registry& reg, gametime::duration delta) override
    {

        for(const auto view = reg.view<const life::begin_die>();
        const auto entity: view)
        {
            reg.destroy(entity);
        }
        
    }
};
