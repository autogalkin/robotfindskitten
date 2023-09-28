#pragma once

#include "engine/details/base_types.h"
#include "engine/world.h"
#include "engine/time.h"

struct lifetime_ticker{
    void execute(entt::registry& reg, const timings::duration delta){
        using namespace std::chrono_literals;
        for (const auto view = reg.view<life::lifetime>();
             const auto entity : view) {
            if (auto& [duration] = view.get<life::lifetime>(entity);
                duration <= 0ms) {

                reg.emplace<life::begin_die>(entity);
            } else {
                duration -= delta;
            }
        }
    }
};

struct death_last_will_executor{
    void execute(entt::registry& reg, timings::duration delta){
        for (const auto view =
                 reg.view<life::begin_die, life::death_last_will>();
             const auto entity : view) {
            view.get<life::death_last_will>(entity).wish(reg, entity);
        }
    }
};

struct timeline_executor{
    void execute(entt::registry& reg, timings::duration delta){

        for (const auto view = reg.view<const timeline::what_do,
                                        const timeline::eval_direction>();
             const auto entity : view) {
            auto& [work] = view.get<const timeline::what_do>(entity);
            work(reg, entity, view.get<timeline::eval_direction>(entity).value);
        }
    }
};
