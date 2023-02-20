#pragma once
#include <entt/entt.hpp>
#include "../core/base_types.h"
#include "../ecs_processors/collision.h"
#include "../core/input.h"

struct projectile
{
    static void make(entt::registry& reg, const entt::entity e
        , location start
        , velocity direction
        ,  const std::chrono::milliseconds life_time=std::chrono::seconds{1})
    {
        reg.emplace<shape>(e, shape::initializer_from_data{"-", 1, 1});
        
        reg.emplace<location_buffer>(e, std::move(start), location{});
        reg.emplace<non_uniform_movement_tag>(e);
        reg.emplace<velocity>(e, direction);
        reg.emplace<collision::agent>(e);
        
        reg.emplace<lifetime>(e, life_time);
        reg.emplace<death_last_will>(e, [](entt::registry& reg_, const entt::entity ent)
        {
            const auto newe = reg_.create();
            reg_.emplace<location_buffer>(newe, reg_.get<location_buffer>(ent));
            reg_.emplace<shape>( newe, shape::initializer_from_data{"*", 1, 1});
            reg_.emplace<lifetime>(newe, std::chrono::seconds{1});
        });
        
    }
};

struct timeline_1
{
    static void make(entt::registry& reg, const entt::entity e
        , const std::function<void(timeline_do::direction)>& what_do
        , const std::chrono::milliseconds duration=std::chrono::seconds{1}
        , timeline_do::direction direction      = timeline_do::direction::forward
 )
    {
        reg.emplace<lifetime>(e, duration);
        reg.emplace<timeline_do>(e, what_do, direction);
    }
};

struct character final
{
    static void make(entt::registry& reg, const entt::entity e, location l)
    {
        reg.emplace<location_buffer>(e, std::move(l), location{});
        reg.emplace<uniform_movement_tag>(e);
        reg.emplace<velocity>(e);
        reg.emplace<collision::agent>(e);
        
    }

    template< input::key UP   =input::key::w
            , input::key LEFT =input::key::a
            , input::key DOWN =input::key::s
            , input::key RIGHT=input::key::d
            , input::key ACTION=input::key::space
    >
    static void process_input(entt::registry& reg, const entt::entity e, const input::key_state_type& state)
    {
        auto& vel = reg.get<velocity>(e);
       
        for(auto& key : state)
        {
            switch (key)
            {
            case UP: vel.line()             -= 1; break;
            case LEFT: vel.index_in_line()  -= 1; break;
            case DOWN: vel.line()           += 1; break;
            case RIGHT: vel.index_in_line() += 1; break;
            case ACTION:
                {
                    auto& loc = reg.get<location_buffer>(e);
                    const auto proj = reg.create();
                    projectile::make(reg, proj,loc.current + location{0, 1},  velocity{0, 15}, std::chrono::seconds{4});

                    const auto t = reg.create();
                    timeline_1::make(reg, t, [](timeline_do::direction direction){gamelog::cout("hello time");}, std::chrono::seconds{3});
                }
            default: break;
            }
        }
    }
};

