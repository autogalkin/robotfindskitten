/**
 * @file
 * @brief All ECS component factories.
 */

#ifndef _CPP_PROJECTS_ROBOTFINDSKITTEN_SRC_ROBOTFINDSKITTEN_GAME_FACTORIES_H
#define _CPP_PROJECTS_ROBOTFINDSKITTEN_SRC_ROBOTFINDSKITTEN_GAME_FACTORIES_H

#include <glm/gtx/easing.hpp>
#include <entt/entity/registry.hpp>
#include <entt/entity/entity.hpp>
#include <entt/entity/handle.hpp>

#include "engine/details/base_types.hpp"
#include "game/systems/collision_query.h"
#include "game/systems/drawers.h"
#include "game/systems/input.h"
#include "game/systems/life.h"
#include "game/systems/motion.h"

namespace factories {

/**
 * @brief Emplaces a minimal set of components for execution in drawing systems
 *
 * @param h A handle to the entity
 * @param start A position in the game
 * @param z_depth_position A depth position
 * @param sprt An entity sprite
 */
inline void make_base_renderable(entt::handle h, loc start,
                                 int z_depth_position, sprite sprt) {
    h.emplace<visible_tag>();
    h.emplace<loc>(start);
    h.emplace<translation>();
    h.emplace<sprite>(std::move(sprt));
    h.emplace<current_rendering_sprite>(h.entity());
    h.emplace<z_depth>(z_depth_position);
}

/**
 * @brief Sets up the animation that will execute after the entity's death.
 *
 * @param h A handle to the entity
 */
void emplace_simple_death_anim(entt::handle h);

} // namespace factories

/**
 * @brief Functions for all projectiles
 */
namespace projectile {

/**
 * @brief A projectile identifier
 */
struct projectile_tag {};

// collision callback
void on_collide(const void* payload, entt::registry& r, collision::self self,
                collision::collider other);

/**
 * @brief Emplaces necessary components for a projectile instance
 *
 * @param h A handle to the entity
 * @param start A projectile start position
 * @param direction An initial force (projectiles work with pseudo non-uniform
 * motion)
 * @param life_time A projectile lifetime
 */
void make(entt::handle h, loc start, velocity direction,
          std::chrono::milliseconds life_time = std::chrono::seconds{1});

/**
 * @brief Sets up shared data for all projectiles into the registry
 *
 * @param reg The game registry
 */
void initialize_for_all(entt::registry& reg);

} // namespace projectile

/**
 * @brief Functions for gun entity
 */
namespace gun {
/**
 * @brief A gun identifier
 */
struct gun_tag {};

// collision callback
void on_collide(const void* payload, entt::registry& reg, collision::self self,
                collision::collider collider);

void make(entt::handle h, loc loc, sprite sp);

/**
 * @brief Executes fire action
 *
 * @param character The player entity
 */
void fire(entt::handle player);
} // namespace gun

/**
 * @brief Functions for objects that execute a task in every tick following a
 * specific direction \ref timeline::direction
 */
namespace timeline {
/**
 * @brief A identifier of the timeline entity
 */
struct timeline_tag {};

/**
 * @brief Emplaces a set of components for working with the timeline system
 *
 * @param h A handle to the entity
 * @param what_do A timeline function for execution in every game tick
 * @param duration A timeline execution duration
 * @param dir A direction of the execution
 */
void make(entt::handle h, timeline::what_do::function_type what_do,
          std::chrono::milliseconds duration = std::chrono::seconds{1},
          direction dir = direction::forward);

}; // namespace timeline

/**
 * @brief Functions for objects that wait the end of own lifetime and call a
 * given function
 */
namespace timer {
/**
 * @brief A timer identifier
 */
struct timer_tag {};

inline void make(entt::handle h,
                 life::dying_wish::function_type what_do_at_the_end,
                 std::chrono::milliseconds duration = std::chrono::seconds{1}) {
    h.emplace<life::life_time>(duration);
    h.emplace<life::dying_wish>(what_do);
}
}; // namespace timer

/**
 * @brief Functions for player entity
 */
namespace character {
/**
 * @brief An identifier of the player entity
 */
struct character_tag {};

// collision callback
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

template<input::key UP = input::key::w, input::key LEFT = input::key::a,
         input::key DOWN = input::key::s, input::key RIGHT = input::key::d>
void process_movement_input(const void* /*unused*/, entt::handle h,
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
/**
 * @brief Branches of the game
 */
enum class game_status_flag {
    unset,
    find,
    kill,
};

} // namespace game_over

namespace kitten {
/**
 * @brief An identifier of the kitten entity
 */
struct kitten_tag {};
// collision callback
void on_collide(const void* /*payload*/, entt::registry& reg,
                collision::self self, collision::collider collider);

void make(entt::handle h, loc loc, sprite sp);

} // namespace kitten

#endif
