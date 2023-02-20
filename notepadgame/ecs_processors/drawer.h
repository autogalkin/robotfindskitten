#pragma once
#include "../core/base_types.h"
#include "../core/world.h"


class drawer final : public ecs_processor
{
public:
    explicit drawer(world* w): ecs_processor(w){
    }

    void execute(entt::registry& reg, gametime::duration delta) override
    {
        for(const auto view = reg.view<location_buffer, const shape>();
        const auto entity: view)
        {
            auto& [current, translation] = view.get<location_buffer>(entity);
            
                get_world()->redraw(position_converter::from_location(current), position_converter::from_location(current + translation), view.get<shape>(entity));
                current += translation;
                translation = location::null();
            
        }
    }
};
