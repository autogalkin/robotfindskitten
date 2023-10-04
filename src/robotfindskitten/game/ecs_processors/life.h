#pragma once

#include "engine/world.h"
#include "engine/time.h"

namespace timeline {
enum class direction : int8_t { forward = 1, reverse = -1 };
// calls a given function every tick to the end lifetime
struct what_do {
    std::function<void(entt::registry&, entt::entity, direction)> work;
};

struct eval_direction {
    direction value;
};
inline direction invert(direction v) {
    return static_cast<direction>(-1 * static_cast<int>(v));
}

struct executor: ecs_proc_tag {
    void execute(entt::registry& reg, timings::duration /*dt*/) {
        for(const auto view = reg.view<const timeline::what_do,
                                       const timeline::eval_direction>();
            const auto entity: view) {
            const auto& [work] = view.get<const timeline::what_do>(entity);
            work(reg, entity, view.get<timeline::eval_direction>(entity).value);
        }
    }
};

} // namespace timeline

namespace life {
struct begin_die {};

struct lifetime {
    timings::duration duration;
};

struct death_last_will {
    std::function<void(entt::registry&, entt::entity)> wish;
};

struct life_ticker: ecs_proc_tag {
    void execute(entt::registry& reg, const timings::duration dt) {
        using namespace std::chrono_literals;
        for(const auto view = reg.view<lifetime>(); const auto entity: view) {
            if(auto& [duration] = view.get<lifetime>(entity); duration <= 0ms) {
                reg.emplace<begin_die>(entity);
            } else {
                duration -= dt;
            }
        }
    }
};

struct death_last_will_executor: ecs_proc_tag {
    void execute(entt::registry& reg, timings::duration /*dt*/) {
        for(const auto view = reg.view<begin_die, death_last_will>();
            const auto entity: view) {
            view.get<life::death_last_will>(entity).wish(reg, entity);
        }
    }
};
struct killer: ecs_proc_tag {
    void execute(entt::registry& reg, timings::duration /*dt*/) {
        for(const auto view = reg.view<const begin_die>();
            const auto entity: view) {
            reg.destroy(entity);
        }
    }
};
} // namespace life
