#pragma once
#include <vector>
#include <chrono>
#include "../world/actor.h"
#include <entt/entt.hpp>


#include "range/v3/view/enumerate.hpp"
#include "range/v3/view/filter.hpp"
#include "range/v3/view/view.hpp"


struct collision
{
    enum class type: int8_t
    {
          ignore = 0
        , block = 1
    };
    std::function<void(entt::entity e, entt::registry& reg)> callback_on_collide{};
};





class collision_query final : public ecs_processor
{
public:
    // TODO пока что просто брутфорс
    virtual void execute(entt::registry& reg) override
    {
        for(const auto view = reg.view<const collision, const location, const shape, velocity>();
            const auto entity: view)
        {
            auto& sh = view.get<shape>(entity);
            auto& loc = view.get<location>(entity);
            auto& coll = view.get<collision>(entity);
            auto& vel = view.get<velocity>(entity);

            location desired_pos = loc + vel;

            //sh.data.rows();
            //    auto &vel = view.get<collider>(entity);
            //    vel.callback_on_collide();
            // do collision and reset velocity if collide
        }
    }
};

class movement final : public ecs_processor
{
public:
    void execute(entt::registry& reg) override
    {
        for(const auto view = reg.view<location, const shape, const velocity>();
            const auto entity: view)
        {
            
            auto& vel = view.get<velocity>(entity);
            if(vel.isZero()) continue;

            const auto& [sh] = view.get<shape>(entity);
            auto& loc = view.get<location>(entity);
            
            loc += vel;

            // draw
            for(auto rows = sh.rowwise();
                auto [line, row] : rows | ranges::views::enumerate)
            {
                for(int ind_in_line{-1}; const auto ch : row
                    | ranges::views::filter([&ind_in_line](const char c){++ind_in_line; return c != shape::whitespace;}))
                {
                    //notepader::get().get_world()->get_level()->at(loc + location{static_cast<npi_t>(line), ind_in_line}) = ch;
                }
            }
        }
    }
};



