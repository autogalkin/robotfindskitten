
#ifndef _CPP_PROJECTS_ROBOTFINDSKITTEN_SRC_ROBOTFINDSKITTEN_GAME_ECS_PROCESSORS_LIFE_H
#define _CPP_PROJECTS_ROBOTFINDSKITTEN_SRC_ROBOTFINDSKITTEN_GAME_ECS_PROCESSORS_LIFE_H

#include <entt/entity/registry.hpp>
#include <entt/entity/handle.hpp>
#include <entt/entity/observer.hpp>
#include <entt/signal/delegate.hpp>

#include "engine/time.h"

namespace timeline {

/*! @brief Possible timeline directions */
enum class direction : int8_t { forward = 1, reverse = -1 };

/**
 * @brief What the timeline entity should do on every tick.
 */
struct what_do {
    using function_type =
        entt::delegate<void(entt::handle, direction, timings::duration)>;
    function_type what_do;
};

/**
 * @brief A type wrapper for the ECS registry that holds a timeline direction
 */
struct eval_direction {
    direction v;
};
/*! @brief invert \p direction */
inline direction invert(direction v) {
    return static_cast<direction>(-1 * static_cast<int>(v));
}

/**
 * @brief The timeline ECS system. Executes \ref what_do on every tick.
 */
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
/**
 * @brief A tag for mark entity which should be deleted and the end of a game
 * tick
 */
struct begin_die {};

/*! @brief A duration of the entity life */
struct life_time {
    timings::duration value;
};

/*! @brief An entity's death callback */
struct dying_wish {
    using function_type = entt::delegate<void(entt::handle)>;
    function_type fullfill;
};

/**
 * @brief A life time system. Decreases a \ref life_time and enplace \ref
 * begin_die
 */
struct life_ticker {
    void operator()(const entt::view<entt::get_t<life_time>>& view,
                    entt::registry& reg, timings::duration dt) {
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

/**
 * @brief The entity's death callback system. Executes an entity's dying wish
 * after begin_die was inserted
 */
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

/**
 * @brief The entity destroy system.
 * Observes a \ref begin_die tag and remove whole entity from the registry
 */
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
