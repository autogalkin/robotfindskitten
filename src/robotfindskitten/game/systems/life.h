#pragma once
#ifndef _CPP_PROJECTS_ROBOTFINDSKITTEN_SRC_ROBOTFINDSKITTEN_GAME_ECS_PROCESSORS_LIFE_H
#define _CPP_PROJECTS_ROBOTFINDSKITTEN_SRC_ROBOTFINDSKITTEN_GAME_ECS_PROCESSORS_LIFE_H

#include "engine/world.h"
#include "engine/time.h"

namespace timeline {
enum class direction : int8_t { forward = 1, reverse = -1 };
// calls a given function every tick to the end lifetime
struct what_do {
    entt::delegate<void(entt::handle, direction, timings::duration)> what_do;
};

struct eval_direction {
    direction value;
};
inline direction invert(direction v) {
    return static_cast<direction>(-1 * static_cast<int>(v));
}

struct executor: is_system {
    explicit executor(entt::registry& reg) {
        [[maybe_unused]] auto first_initialize_group =
            reg.group<const timeline::what_do,
                      const timeline::eval_direction>();
    }
    void operator()(entt::registry& reg, const timings::duration dt) {
        for(; const auto [entity, what_do, dir]:
              reg.group<const timeline::what_do,
                        const timeline::eval_direction>) {
            what_do(entt::handle{reg, entity}, dir, dt);
        }
    }
};

} // namespace timeline

namespace life {
struct begin_die {};

struct life_time {
    timings::duration value;
};

struct dying_wish {
    entt::delegate<void(entt::handle)> fullfill;
};

struct life_ticker: is_system {
    void operator()(entt::view<entt::get_t<life_time>> view,
    entt::registry& reg, const timings::duration dt) {
        using namespace std::chrono_literals;
        view.each([](auto ent, auto& lt){
            if(lt.value > 0ms) {
                lt.value -= dt;
            } else {
                reg.emplace<begin_die>(ent);
            }

        });
    }
};

struct fullfill_dying_wish: is_system {
    explicit fullfill_dying_wish(entt::registry& reg) {
        [[maybe_unused]] auto first_initialize_group =
            reg.group<dying_wish>(entt::get<begin_die>);
    }
    void operator()(entt::registry& reg) {
        for(const auto& [entity, wish]:
            reg.group<dying_wish>(entt::get<begin_die>)) {
            wish(entt::handle{entity, reg});
        }
    }
};

class killer: public is_system {
    entt::observer obs_;

public:
    explicit killer(entt::registry& reg)
        : obs_(reg, entt::collector.group<begin_die>()) {}
    void operator()(entt::registry& reg) {
        obs_.each([&reg](const auto ent) { reg.destroy(ent); });
        obs_.clear();
    }
};
} // namespace life

#endif
