#pragma once


#include <entt/entt.hpp>
#include "../core/base_types.h"
#include "../core/world.h"




class lifetime_ticker final : public ecs_processor
{
public:
    explicit lifetime_ticker(world* w): ecs_processor(w){
    }

    void execute(entt::registry& reg, gametime::duration delta) override
    {
        using namespace std::chrono_literals;
        for(const auto view = reg.view<lifetime>();
        const auto entity: view)
        {
            if(auto& [life_time] = view.get<lifetime>(entity)
                ; life_time <= 0ms){
                
                if(reg.all_of<shape, location_buffer>(entity)){
                    get_world()->erase(position_converter::from_location(reg.get<location_buffer>(entity).current), reg.get<shape>(entity));
                }
                reg.destroy(entity);
                
            }
            else
            {
                life_time -= delta;
            }
            
        }
    }
};

struct timeline_
{
    enum class direction : int8_t{
        forward = 1,
        reverse = -1
    };
    std::function<void()> work;
    direction d{direction::forward};
};

class timeline_executor final : ecs_processor
{
public:
    explicit timeline_executor(world* w)
        : ecs_processor(w)
    {}

    void execute(entt::registry& reg, gametime::duration delta) override
    {
        for(const auto view = reg.view<lifetime, timeline_>();
        const auto entity: view)
        {
            view.get<timeline_>(entity).work();
        }
    }
};