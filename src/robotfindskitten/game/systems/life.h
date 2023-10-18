#pragma once
#ifndef _CPP_PROJECTS_ROBOTFINDSKITTEN_SRC_ROBOTFINDSKITTEN_GAME_ECS_PROCESSORS_LIFE_H
#define _CPP_PROJECTS_ROBOTFINDSKITTEN_SRC_ROBOTFINDSKITTEN_GAME_ECS_PROCESSORS_LIFE_H

#include <entt/entt.hpp>

#include "engine/time.h"

namespace timeline {
enum class direction : int8_t { forward = 1, reverse = -1 };
// calls a given function every tick to the end lifetime
struct what_do {
    using function_type =
        entt::delegate<void(entt::handle, direction, timings::duration)>;
    function_type what_do;
};

struct eval_direction {
    direction v;
};
inline direction invert(direction v) {
    return static_cast<direction>(-1 * static_cast<int>(v));
}

struct timeline_step {
    explicit timeline_step(entt::registry& reg) {
        [[maybe_unused]] auto first_initialize_group =
            reg.group<const timeline::what_do,
                      const timeline::eval_direction>();
    }
    void operator()(entt::registry& reg, const timings::duration dt) {
        for(const auto& [entity, what_do, dir]:
            reg.group<const timeline::what_do, const timeline::eval_direction>()
                .each()) {
            what_do.what_do(entt::handle{reg, entity}, dir.v, dt);
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
    using function_type = entt::delegate<void(entt::handle)>;
    function_type fullfill;
};

struct life_ticker {
    void operator()(const entt::view<entt::get_t<life_time>>& view,
                    entt::registry& reg, const timings::duration dt) {
        using namespace std::chrono_literals;
        view.each([dt, &reg](auto ent, auto& lt) {
            if(lt.value > 0ms) {
                lt.value -= dt;
            } else {
                reg.emplace<begin_die>(ent);
            }
        });
    }
};

struct fullfill_dying_wish {
    explicit fullfill_dying_wish(entt::registry& reg) {
        [[maybe_unused]] auto first_initialize_group =
            reg.group<dying_wish>(entt::get<begin_die>);
    }
    void operator()(entt::registry& reg) {
        for(const auto& [ent, wish]:
            reg.group<dying_wish>(entt::get<begin_die>).each()) {
            wish.fullfill(entt::handle{reg, ent});
        }
    }
};

class killer {
    entt::observer obs_;

public:
    explicit killer(entt::registry& reg)
        // NOLINTNEXTLINE(readability-static-accessed-through-instance)
        : obs_(reg, entt::collector.group<begin_die>()) {}
    void operator()(entt::registry& reg) {
        reg.destroy(obs_.begin(), obs_.end());
        obs_.clear();
    }
};
} // namespace life

#endif
