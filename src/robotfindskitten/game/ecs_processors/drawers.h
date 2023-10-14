#pragma once
#ifndef _CPP_PROJECTS_ROBOTFINDSKITTEN_SRC_ROBOTFINDSKITTEN_GAME_ECS_PROCESSORS_DRAWERS_H
#define _CPP_PROJECTS_ROBOTFINDSKITTEN_SRC_ROBOTFINDSKITTEN_GAME_ECS_PROCESSORS_DRAWERS_H

#include <mutex>

#include <glm/vector_relational.hpp>

#include "engine/buffer.h"
#include "engine/details/base_types.hpp"
#include "engine/world.h"
#include "game/comps.h"
#include "game/ecs_processors/life.h"
#include "game/ecs_processors/motion.h"

enum class draw_direction {
    left = -1,
    right = 1,
};
inline draw_direction invert(draw_direction s) {
    return static_cast<draw_direction>(-1 * static_cast<int>(s));
}

struct on_change_direction {
    std::function<void(draw_direction new_direction, sprite&)> call;
};
class rotate_animator: ecs_proc_tag {
public:
    void execute(entt::registry& reg, timings::duration /*dt*/) {
        for(const auto view = reg.view<loc, translation, sprite,
                                       on_change_direction, draw_direction>();
            const auto entity: view) {
            auto& trans = view.get<translation>(entity);

            if(auto& dir = view.get<draw_direction>(entity);
               trans.get().x != 0
               && static_cast<int>(std::copysign(1, trans.get().x))
                      != static_cast<int>(dir)) {
                dir = invert(dir);
                view.get<on_change_direction>(entity).call(
                    dir, view.get<sprite>(entity));

                trans.pin().x = 0.;
            }
        }
    }
};

struct z_depth {
    int32_t index;
};
struct previous_sprite {
    sprite sp;
};

template<typename BufferType>
    requires requires(BufferType& b, pos p, sprite_view w, int z_depth) {
        { b.erase(p, w, z_depth) };
        { b.draw(p, w, z_depth) };
        { b.lock() };
    }
class redrawer: ecs_proc_tag {
    std::reference_wrapper<BufferType> buf_;

public:
    explicit redrawer(BufferType& buf, world& w): buf_(buf) {
        w.registry.on_construct<visible_in_game>()
            .connect<&redrawer::upd_visible>();
    }

    void execute(entt::registry& reg, timings::duration /*dt*/) {
        // a mutex lock for draw and erase functions together
        auto lock = buf_.get().lock();
        for(const auto view =
                reg.view<const loc, const previous_sprite, const z_depth>();
            const auto entity: view) {
            const auto& [sprt] = view.get<previous_sprite>(entity);
            buf_.get().erase(view.get<loc>(entity), sprt,
                             view.get<z_depth>(entity).index);
            reg.remove<previous_sprite>(entity);
        }
        for(const auto view =
                reg.view<const loc, const translation, const visible_in_game,
                         const sprite, const z_depth>();
            const auto entity: view) {
            const bool is_dead = reg.all_of<life::begin_die>(entity);
            const auto& trans = view.get<const translation>(entity);
            if(trans.is_changed()
               || glm::all(glm::greaterThanEqual(trans.get(), loc(1)))
               || is_dead) {
                buf_.get().erase(view.get<loc>(entity),
                                 view.get<sprite>(entity),
                                 view.get<z_depth>(entity).index);
                if(is_dead) {
                    reg.remove<visible_in_game>(entity);
                }
            }
        }

        for(const auto view = reg.view<loc, translation, const sprite,
                                       const visible_in_game, const z_depth>();
            const auto entity: view) {
            auto& current = view.get<loc>(entity);
            auto& trans = view.get<translation>(entity);

            if(trans.is_changed()
               || glm::all(glm::greaterThanEqual(trans.get(), loc(1)))) {
                const auto& sp = view.get<sprite>(entity);
                current += trans.get();
                buf_.get().draw(current, sp, view.get<z_depth>(entity).index);
                trans = loc(0);
                trans.clear();
            }
        }
    }

private:
    static void upd_visible(entt::registry& reg, const entt::entity e) {
        if(auto* const ptr = reg.try_get<translation>(e)) {
            ptr->mark();
        }
    }
};

#endif
