#pragma once
#ifndef _CPP_PROJECTS_ROBOTFINDSKITTEN_SRC_ROBOTFINDSKITTEN_GAME_ECS_PROCESSORS_DRAWERS_H
#define _CPP_PROJECTS_ROBOTFINDSKITTEN_SRC_ROBOTFINDSKITTEN_GAME_ECS_PROCESSORS_DRAWERS_H

#include <mutex>

#include <glm/vector_relational.hpp>

#include "engine/buffer.h"
#include "engine/details/base_types.hpp"
#include "engine/world.h"
#include "game/comps.h"
#include "game/systems/life.h"
#include "game/systems/motion.h"

enum class draw_direction {
    left = -1,
    right = 1,
};
inline draw_direction invert(draw_direction s) {
    return static_cast<draw_direction>(-1 * static_cast<int>(s));
}

struct current_rendering_sprite {
    entt::entity where = entt::null;
};

template<draw_direction Direction>
struct direction_sprite {
    entt::entity where = entt::null;
};

class rotate_animator: is_system {
public:
    void operator()(
        entt::view<
            entt::get_t<current_rendering_sprite, draw_direction, translation,
                        const direction_sprite<draw_direction::right>,
                        const direction_sprite<draw_direction::left>>>
            view) {
        for(const auto ent: view) {
            auto& trans = view.get<translation>(ent);

            if(auto& dir = view.get<draw_direction>(ent);
               trans.get().x != 0
               && static_cast<int>(std::copysign(1, trans.get().x))
                      != static_cast<int>(dir)) {
                dir = invert(dir);
                auto& [cur_sprt] = view.get<current_rendering_sprite>(ent);
                switch(dir) {
                case draw_direction::left: {
                    cur_sprt =
                        view.get<const direction_sprite<draw_direction::left>>(
                                ent)
                            .where;
                    break;
                }
                case draw_direction::right: {
                    cur_sprt =
                        view.get<const direction_sprite<draw_direction::right>>(
                                ent)
                            .where;
                    break;
                }
                }
                trans.pin().x = 0.;
            }
        }
    }
};

struct z_depth {
    int32_t index;
};
struct previous_sprite {
    entt::entity where = entt::null;
};
struct visible_tag {};
struct need_redraw_tag {};

struct apply_translation: is_system {
    void operator()(entt::view<entt::get_t<loc, translation>> view,
                    entt::registry& reg) {
        for(auto& [ent, loc, trans]: view.each()) {
            if(trans.is_changed()
               && glm::all(glm::greaterThanEqual(trans.get(), loc(1)))) {
                loc += std::exchange(trans.pin(), loc(0));
                trans.clear();
                reg.emplace_or_replace<need_redraw_tag>(ent);
            }
        }
    }
}

template<typename BufferType>
    requires requires(BufferType& b, pos p, sprite_view w, int z_depth) {
        { b.erase(p, w, z_depth) };
        { b.draw(p, w, z_depth) };
        { b.lock() };
    }
class redrawer: is_system {
    std::reference_wrapper<BufferType> buf_;

public:
    explicit redrawer(BufferType& buf, world& w): buf_(buf) {
        w.registry.on_construct<visible_tag>()
            .connect<&entt::registry::emplace_or_replace<need_redraw_tag>>();
        w.registry.on_construct<life::begin_die>()
            .connect<&entt::registry::emplace_or_replace<need_redraw_tag>>();
        w.registry.on_destroy<visible_tag>()
            .connect<&entt::registry::emplace_or_replace<need_redraw_tag>>();
    }
    void operator()(
        entt::view<entt::get_t<const loc, const previous_sprite, const z_depth>>
            erase_prev,
        entt::view<
            entt::get_t<const loc, const current_rendering_sprite, const sprite,
                        const z_depth, const need_redraw_tag>>
            erase_updated,
        entt::view<entt::get_t<const loc, const current_rendering_sprite,
                               const visible_tag, const z_depth,
                               const need_redraw_tag>,
                   entt::exclude_t<const life::begin_die>>
            draw,
        entt::view<entt::get_t<const sprite>> sprites) {
        // a mutex lock for draw and erase together
        auto lock = buf_.get().lock();

        for(const auto& [ent, loc, prev_sprt, zd]: erase_prev.each()) {
            buf_.get().erase(loc, sprites.get<const sprite>(prev_sprt.where),
                             zd.index);
        }
        erase_updated.each([](auto ent, loc l, current_rendering_sprite s,
                              z_depth z) {
            buf_.get().erase(l, sprites.get<const sprite>(s.where), z.index);
        });
        draw.each(
            [](auto ent, loc l, const current_rendering_sprite s, z_depth z) {
                buf_.get().draw(l, sprites.get<const sprite>(s.where), z.index);
            });
    }
};

struct redraw_cleanup: is_system {
    void operator()(entt::registry& reg) {
        reg.clear<previous_sprite>();
        reg.clear<need_redraw_tag>();
    }
};

#endif
