#pragma once
#include <entt/entt.hpp>
#include "../core/base_types.h"
#include "../ecs_processors/collision.h"
#include "../core/input.h"


struct character final
{
    static void make(entt::registry& reg, const entt::entity e, location l)
    {
        reg.emplace<location>(e, std::move(l));
        reg.emplace<velocity>(e);
        reg.emplace<collision::agent>(e);
        reg.emplace<acceleration>(e);
        
    }

    template< input::key UP   =input::key::w
            , input::key LEFT =input::key::a
            , input::key DOWN =input::key::s
            , input::key RIGHT=input::key::d
    >
    static void process_input(entt::registry& registry, const entt::entity e, const input::key_state_type& state)
    {
        auto& vel = registry.get<velocity>(e);
        const auto& [force] = registry.get<const acceleration>(e);
        for(auto& key : state)
        {
            switch (key)
            {
            case UP: vel.line()             -= force; break;
            case LEFT: vel.index_in_line()  -= force; break;
            case DOWN: vel.line()           += force; break;
            case RIGHT: vel.index_in_line() += force; break;
            default: break;
            }
        }
    }
};
