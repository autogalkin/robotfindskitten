#pragma once


#include <entt/entt.hpp>
#include "../core/base_types.h"
#include "../core/world.h"




class lifetime_ticker final : public ecs_processor
{
public:
    explicit lifetime_ticker(world* w): ecs_processor(w){
    }

    void execute(entt::registry& reg, const gametime::duration delta) override
    {
        using namespace std::chrono_literals;
        for(const auto view = reg.view<lifetime>();
        const auto entity: view)
        {
            if(auto& [duration] = view.get<lifetime>(entity)
                ; duration <= 0ms){
                
                reg.emplace<begin_die>(entity);
            }
            else
            {
                duration -= delta;
            }
            
        }
    }
};

class death_last_will_executor final : public ecs_processor
{
public:
    explicit death_last_will_executor(world* w)
        : ecs_processor(w){
    }

    void execute(entt::registry& reg, gametime::duration delta) override
    {
        for(const auto view = reg.view<begin_die, death_last_will>();
        const auto entity: view)
        {
            view.get<death_last_will>(entity).wish(reg, entity);
        }
    }
};

class timeline_executor final : public ecs_processor
{
public:
    explicit timeline_executor(world* w)
        : ecs_processor(w)
    {}

    void execute(entt::registry& reg, gametime::duration delta) override
    {
        
        for(const auto view = reg.view<const lifetime, timeline_do>();
        const auto entity: view)
        {
            auto& d = view.get<const timeline_do>(entity);
            auto& [work, dir] = view.get<const timeline_do>(entity);
            work( dir);
        }
    }
};