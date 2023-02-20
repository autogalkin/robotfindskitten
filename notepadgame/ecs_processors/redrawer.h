#pragma once
#include "../core/base_types.h"
#include "../core/world.h"


class redrawer final : public ecs_processor
{
public:
    explicit redrawer(world* w): ecs_processor(w){
    }

    void execute(entt::registry& reg, gametime::duration delta) override
    {
        
        for(const auto view = reg.view<location_buffer, const shape>();
        const auto entity: view)
        {
            auto& [current, translation] = view.get<location_buffer>(entity);

            get_world()->erase(position_converter::from_location(view.get<location_buffer>(entity).current), view.get<shape>(entity));
            
            if(!reg.all_of<begin_die>(entity))
            {
                get_world()->draw(position_converter::from_location(current + translation), view.get<shape>(entity));
            }
            current += translation;
            translation = location::null();
            
        }
    }
};

