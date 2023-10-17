#pragma once
#ifndef _CPP_PROJECTS_ROBOTFINDSKITTEN_SRC_ROBOTFINDSKITTEN_GAME_FACTORIES_H
#define _CPP_PROJECTS_ROBOTFINDSKITTEN_SRC_ROBOTFINDSKITTEN_GAME_FACTORIES_H

#include <glm/gtx/easing.hpp>

#include "engine/details/base_types.hpp"
#include "engine/time.h"
#include "game/systems/collision_query.h"
#include "game/systems/drawers.h"
#include "game/systems/input.h"
#include "game/systems/life.h"
#include "game/systems/motion.h"

namespace factories {
struct actor_tag {};
inline void make_base_renderable(entt::handle h, loc start,
                                 int z_depth_position, sprite sprt) {
    h.emplace<visible_tag>();
    h.emplace<loc>(start);
    h.emplace<translation>();
    h.emplace<sprite>(std::move(sprt));
    h.emplace<current_rendering_sprite>(h.entity());
    h.emplace<z_depth>(z_depth_position);
}

void emplace_simple_death_anim(entt::handle h);
} // namespace factories

namespace projectile {
struct projectile_tag {};
void on_collide(const void* payload, entt::registry& r, collision::self self,
                collision::collider other);
void make(entt::handle h, loc start, velocity direction,
          std::chrono::milliseconds life_time = std::chrono::seconds{1});
void initialize_for_all(entt::registry& reg);
} // namespace projectile

namespace gun {
struct gun_tag {};
void on_collide(const void* payload, entt::registry& reg, collision::self self,
                collision::collider collider);

void make(entt::handle h, loc loc, sprite sp);

void fire(entt::handle character);
} // namespace gun

namespace timeline {
struct timeline_tag {};
void make(entt::handle h, timeline::what_do::function_type what_do,
          std::chrono::milliseconds duration = std::chrono::seconds{1},
          direction dir = direction::forward);

}; // namespace timeline

// waits for the end of time and call a given function
namespace timer {
struct timer_tag {};
inline void make(entt::handle h, life::dying_wish::function_type what_do,
                 std::chrono::milliseconds duration = std::chrono::seconds{1}) {
    h.emplace<life::life_time>(duration);
    h.emplace<life::dying_wish>(what_do);
}
}; // namespace timer

namespace character {
struct character_tag {};
void on_collide(const void* payload, entt::registry& reg, collision::self self,
                collision::collider collider);

inline void make(entt::handle h, loc l, sprite sprt) {
    h.emplace<character_tag>();
    h.emplace<z_depth>(2);
    h.emplace<collision::hit_extends>(sprt.bounds());
    h.emplace<sprite>(sprt);
    h.emplace<current_rendering_sprite>(h.entity());
    h.emplace<draw_direction>(draw_direction::right);
    h.emplace<visible_tag>();
    h.emplace<loc>(l);
    h.emplace<translation>();
    h.emplace<uniform_movement_tag>();
    h.emplace<velocity>();
    h.emplace<collision::agent>();
    h.emplace<collision::responce_func>(&character::on_collide);
}

// FIXME(Igor): a strange constexpr key setup
template<input::key UP = input::key::w, input::key LEFT = input::key::a,
         input::key DOWN = input::key::s, input::key RIGHT = input::key::d>
void process_movement_input(const void*, entt::handle h,
                            std::span<input::key_state> state) {
    auto& [vel] = h.get<velocity>();
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
void on_collide(const void* payload, entt::registry& reg, collision::self self,
                collision::collider collider);

void make(entt::handle h, loc loc, sprite sp);

} // namespace kitten

namespace atmosphere {
struct atmospere_tag {};
struct color_range {
    COLORREF start{RGB(0, 0, 0)};
    COLORREF end{RGB(255, 255, 255)};
};
void update_cycle(const void* /*payload*/, entt::handle h,
                  timeline::direction d, timings::duration dr);
void make(entt::handle h);

} // namespace atmosphere

#endif
