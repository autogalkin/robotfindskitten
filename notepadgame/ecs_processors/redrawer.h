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
        std::vector<entt::entity> traversed;
        {
            const auto view = reg.view<const location_buffer, const shape::sprite, const begin_die>();
            traversed.reserve(view.size_hint());
            for(const auto entity: view)
            {
                get_world()->erase(position_converter::from_location(view.get<location_buffer>(entity).current), view.get<shape::sprite>(entity));
                traversed.push_back(entity);
            }
        }
        for(const auto view = reg.view<location_buffer, shape::sprite, shape::inverse_sprite, shape::render_direction>();
        const auto entity: view)
        {
            auto& [current, translation] = view.get<location_buffer>(entity);
            auto& [dir] = view.get<shape::render_direction>(entity);
            if(translation.is_changed() && std::ranges::find(traversed, entity) == traversed.end()){
                
                get_world()->erase(position_converter::from_location(view.get<location_buffer>(entity).current), view.get<shape::sprite>(entity));

                auto& sh = view.get<shape::sprite>(entity);

                if(translation.get().index_in_line() != 0 && static_cast<int>(std::copysign(1, translation.get().index_in_line())) != static_cast<int>(dir))
                {
                    std::swap(sh.data,  view.get<shape::inverse_sprite>(entity).data);
                    dir = direction_converter::invert(dir);
                }
                else
                {
                    current += translation.get();
                }
                
                get_world()->draw (position_converter::from_location(current), sh);
                translation = dirty_flag<location>{location::null(), false};
            }
        }
    }
    
};

