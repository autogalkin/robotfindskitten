/**
 * @file
 * @brief Systems that works with the game_buffer.
 */

#ifndef _CPP_PROJECTS_ROBOTFINDSKITTEN_SRC_ROBOTFINDSKITTEN_GAME_ECS_PROCESSORS_DRAWERS_H
#define _CPP_PROJECTS_ROBOTFINDSKITTEN_SRC_ROBOTFINDSKITTEN_GAME_ECS_PROCESSORS_DRAWERS_H

#include <type_traits>
#include <mutex>

#include <glm/vector_relational.hpp>
#include <entt/entity/registry.hpp>
#include <entt/entity/entity.hpp>

#include "engine/details/base_types.hpp"
#include "game/systems/life.h"
#include "game/systems/motion.h"

namespace drawing {

/*! @brief Possible sprite directions */
enum class draw_direction {
    left = -1,
    right = 1,
};

/*! @brief Inverts sprite direction */
inline draw_direction invert(draw_direction s) {
    return static_cast<draw_direction>(-1 * static_cast<int>(s));
}

/*! @brief A sprite id in the sprites entt::registry storage */
struct current_rendering_sprite {
    entt::entity where = entt::null;
};

/*! @brief An id of the sprite that needs to be removed from the entt::registry
 * sprites storage*/
struct sprite_to_destroy {
    entt::entity where = entt::null;
};

/*! @brief If the sprite changes, this component should store the previous
 * sprite to ensure proper erasure in the game buffer. */
struct previous_sprite {
    entt::entity where = entt::null;
};

/*! @brief An id of the sprite that represent a direction */
template<draw_direction Direction>
struct direction_sprite {
    entt::entity where = entt::null;
};

/*! @brief A depth index of the entity */
struct z_depth {
    int32_t index;
};

/*! @brief Should the system render the entity or not? */
struct visible_tag {};

/**
 * @brief Inverts sprites by direction, set the \ref direction_sprite to the
 * \ref current_rendering_sprite. And the \ref current_rendering sprite to the
 * \ref previous_sprite
 */
class rotate_animator {
public:
    void operator()(
        const entt::view<
            entt::get_t<current_rendering_sprite, draw_direction, translation,
                        const direction_sprite<draw_direction::right>,
                        const direction_sprite<draw_direction::left>>>& view) {
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

/*! @brief Sets up by \ref check_need_update system if the entity's translation
 * is changed */
struct need_redraw_tag {};

template<typename BufferType>
concept IsBuffer = requires(BufferType& b, pos p, sprite_view w, int z_depth) {
    { b.erase(p, w, z_depth) };
    { b.draw(p, w, z_depth) };
    { b.lock() };
};

/**
 * @brief A helper class that locks the game buffer mutex before the erase
 * system and the redraw system execute.
 */
template<typename BufferType>
    requires IsBuffer<BufferType>
class lock_buffer {
    std::reference_wrapper<BufferType> buf_;
    using lock_type = decltype(std::declval<BufferType&>().lock());

public:
    explicit lock_buffer(BufferType& buf, entt::registry& reg): buf_(buf) {
        reg.ctx().emplace_as<std::optional<lock_type>>(
            entt::hashed_string{"buffer_lock"});
    }
    void operator()(entt::registry& reg) {
        reg.ctx()
            .get<std::optional<lock_type>>(entt::hashed_string{"buffer_lock"})
            .emplace(buf_.get().lock());
    }
};

/**
 * @brief Inserts \ref need_redraw_tag into the moved entities
 */
struct check_need_update {
    void operator()(const entt::view<entt::get_t<const translation>>& view,
                    entt::registry& reg) {
        view.each([&reg](auto ent, const translation& trans) {
            if(trans.is_changed()) {
                reg.emplace_or_replace<need_redraw_tag>(ent);
            }
        });
    }
};

/**
 * @brief Erases previous positions and sprites from the game buffer
 */
template<typename BufferType>
    requires IsBuffer<BufferType>
class erase_buffer {
    std::reference_wrapper<BufferType> buf_;

public:
    explicit erase_buffer(BufferType& buf): buf_(buf) {}
    void operator()(
        const entt::view<entt::get_t<const loc, const previous_sprite,
                                     const z_depth>>& erase_prev,
        const entt::view<entt::get_t<const loc, const current_rendering_sprite,
                                     const z_depth, const need_redraw_tag>>&
            erase_updated,
        const entt::view<entt::get_t<const sprite>>& sprites) {
        for(const auto& [ent, loc, prev_sprt, zd]: erase_prev.each()) {
            buf_.get().erase(loc, sprites.get<const sprite>(prev_sprt.where),
                             zd.index);
        }
        for(auto e: erase_updated) {
            buf_.get().erase(
                erase_updated.get<const loc>(e),
                sprites.get<const sprite>(
                    erase_updated.get<const current_rendering_sprite>(e).where),
                erase_updated.get<const z_depth>(e).index);
        }
    }
};

/**
 * @brief Apply the entity translation into the entity location for the moved
 * entities.
 */
struct apply_translation {
    void operator()(
        const entt::view<entt::get_t<loc, translation, const need_redraw_tag>>&
            view) {
        view.each([](auto /*ent*/, loc& l, translation& trans) {
            l += std::exchange(trans.pin(), loc(0));
            trans.clear();
        });
    }
};

/**
 * @brief Draws \ref need_redraw_tag sprites into the game buffer
 */
template<typename BufferType>
    requires IsBuffer<BufferType>
class redraw {
    std::reference_wrapper<BufferType> buf_;

public:
    explicit redraw(BufferType& buf, entt::registry& reg): buf_(buf) {
        reg.on_construct<visible_tag>()
            .connect<&entt::registry::emplace_or_replace<need_redraw_tag>>();
        reg.on_construct<life::begin_die>()
            .connect<&entt::registry::emplace_or_replace<need_redraw_tag>>();
        // TODO(Igor): its not clear, because it also invokes 'need_redraw_tag'
        // to the destroyed entity by registry.destroy(entity) and fails
        // reg.on_destroy<visible_tag>()
        //     .connect<&entt::registry::emplace_or_replace<need_redraw_tag>>();
    }
    void operator()(
        const entt::view<entt::get_t<const loc, const current_rendering_sprite,
                                     const visible_tag, const z_depth,
                                     const need_redraw_tag>,
                         entt::exclude_t<const life::begin_die>>& draw,
        const entt::view<entt::get_t<const sprite>>& sprites) {
        for(auto e: draw) {
            buf_.get().draw(
                draw.get<const loc>(e),
                sprites.get<const sprite>(
                    draw.get<const current_rendering_sprite>(e).where),
                draw.get<const z_depth>(e).index);
        }
    }
};

/**
 * @brief Cleans up \ref need_redraw_tag and \ref previous_sprite components
 * from the entire  registry. Removes each sprite sprite marked for destruction
 * in  \ref sprite_to_destroy.
 */
template<typename BufferType>
    requires IsBuffer<BufferType>
struct cleanup {
    void operator()(const entt::view<entt::get_t<const sprite_to_destroy>>&
                        garbage_sprites_view,
                    entt::registry& reg) {
        reg.ctx()
            .get<std::optional<decltype(std::declval<BufferType&>().lock())>>(
                entt::hashed_string{"buffer_lock"}) = std::nullopt;
        for(auto&& [ent, sprt]: garbage_sprites_view.each()) {
            reg.erase<sprite>(sprt.where);
        }
        reg.clear<sprite_to_destroy>();
        reg.clear<previous_sprite>();
        reg.clear<need_redraw_tag>();
    }
};
} // namespace drawing

#endif
