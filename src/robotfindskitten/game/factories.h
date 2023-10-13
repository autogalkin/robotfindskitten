#pragma once
#ifndef _CPP_PROJECTS_ROBOTFINDSKITTEN_SRC_ROBOTFINDSKITTEN_GAME_FACTORIES_H
#define _CPP_PROJECTS_ROBOTFINDSKITTEN_SRC_ROBOTFINDSKITTEN_GAME_FACTORIES_H

#include <glm/gtx/easing.hpp>

#include "engine/details/base_types.hpp"
#include "engine/time.h"
#include "game/comps.h"
#include "game/ecs_processors/collision_query.h"
#include "game/ecs_processors/drawers.h"
#include "game/ecs_processors/input.h"
#include "game/ecs_processors/life.h"
#include "game/ecs_processors/motion.h"

namespace factories {
struct actor_tag {};
inline void make_base_renderable(entt::registry& reg, const entt::entity e,
                                 loc start, int z_depth_position, sprite sprt) {
    reg.emplace<visible_in_game>(e);
    reg.emplace<loc>(e, start);
    reg.emplace<translation>(e);
    reg.emplace<sprite>(e, std::move(sprt));
    reg.emplace<z_depth>(e, z_depth_position);
}

void emplace_simple_death_anim(entt::registry& reg, entt::entity e);
} // namespace factories

namespace projectile {
struct projectile_tag {};
collision::responce on_collide(entt::registry& r, collision::self self,
                               collision::collider other);
void make(entt::registry& reg, entt::entity e, loc start, velocity direction,
          std::chrono::milliseconds life_time = std::chrono::seconds{1});
} // namespace projectile

namespace gun {
struct gun_tag {};
collision::responce on_collide(entt::registry& reg, collision::self self,
                               collision::collider collider);

void make(entt::registry& reg, entt::entity e, loc loc, sprite sp);

void fire(entt::registry& reg, entt::entity character);
} // namespace gun

namespace timeline {
struct timeline_tag {};
void make(
    entt::registry& reg, entt::entity e,
    std::function<void(entt::registry&, entt::entity, direction)>&& what_do,
    std::chrono::milliseconds duration = std::chrono::seconds{1},
    direction dir = direction::forward);
}; // namespace timeline

// waits for the end of time and call a given function
namespace timer {
struct timer_tag {};
inline void make(entt::registry& reg, entt::entity e,
                 std::function<void(entt::registry&, entt::entity)> what_do,
                 std::chrono::milliseconds duration = std::chrono::seconds{1}) {
    reg.emplace<life::lifetime>(e, duration);
    reg.emplace<life::death_last_will>(e, std::move(what_do));
}
}; // namespace timer

namespace character {
struct character_tag {};
collision::responce on_collide(entt::registry& reg, collision::self self,
                               collision::collider collider);

inline void make(entt::registry& reg, entt::entity e, loc l) {
    reg.emplace<character_tag>(e);
    reg.emplace<z_depth>(e, 2);
    reg.emplace<draw_direction>(e, draw_direction::right);
    reg.emplace<visible_in_game>(e);
    reg.emplace<loc>(e, l);
    reg.emplace<translation>(e);
    reg.emplace<uniform_movement_tag>(e);
    reg.emplace<velocity>(e);
    reg.emplace<collision::agent>(e);
    reg.emplace<collision::on_collide>(e, &character::on_collide);
}

// FIXME(Igor): a strange constexpr key setup
template<input::key UP = input::key::w, input::key LEFT = input::key::a,
         input::key DOWN = input::key::s, input::key RIGHT = input::key::d>
void process_movement_input(entt::registry& reg, const entt::entity e,
                            const input::state_t& state,
                            const timings::duration /*dt*/) {
    auto& vel = reg.get<velocity>(e);
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters,
    auto upd = [](double& vel, int value, int pressed_count) mutable {
        static constexpr double pressed_count_end = 7.;
        vel = value
              * glm::sineEaseIn(std::min(pressed_count_end,
                                         static_cast<double>(pressed_count))
                                / pressed_count_end);
    };
    for(auto k: state) {
        switch(k.key) {
        case UP:
            upd(vel.y, -1, k.press_count);
            break;
        case DOWN:
            upd(vel.y, 1, k.press_count);
            break;
        case LEFT:
            upd(vel.x, -1, k.press_count);
            break;
        case RIGHT:
            upd(vel.x, 1, k.press_count);
            break;
        default:
            break;
        }
    }
}
} // namespace character

namespace game_over {

enum class game_status_flag {
    unset,
    find,
    kill,
};

} // namespace game_over

namespace kitten {
struct kitten_tag {};
collision::responce on_collide(entt::registry& reg, collision::self self,
                               collision::collider collider,
                               game_over::game_status_flag& game_over_flag,
                               entt::entity character_entity);

void make(entt::registry& reg, entt::entity e, loc loc, sprite sp,
          game_over::game_status_flag& game_over_flag,
          entt::entity character_entity);

} // namespace kitten

namespace atmosphere {
struct atmospere_tag {};
struct color_range {
    COLORREF start{RGB(0, 0, 0)};
    COLORREF end{RGB(255, 255, 255)};
};
void update_cycle(entt::registry& reg, entt::entity e, timeline::direction d);
void run_cycle(entt::registry& reg, entt::entity timer);
void make(entt::registry& reg, entt::entity e);

} // namespace atmosphere

#endif
