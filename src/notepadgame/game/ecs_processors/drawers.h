#pragma once
#include "df/dirtyflag.h"

#include "engine/details/base_types.h"
#include "engine/world.h"
#include "game/ecs_processors/motion.h"
#include "game/ecs_processors/life.h"
#include "game/comps.h"
#include <glm/vector_relational.hpp>

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
class rotate_animator {
  public:
    explicit rotate_animator() {}

    void execute(entt::registry& reg, timings::duration delta) {

        for (const auto view = reg.view<loc, translation, sprite,
                                        on_change_direction, draw_direction>();
             const auto entity : view) {
            auto& current = view.get<loc>(entity);
            auto& trans = view.get<translation>(entity);

            if (auto& dir = view.get<draw_direction>(entity);
                trans.get().x != 0 &&
                static_cast<int>(std::copysign(1, trans.get().x)) !=
                    static_cast<int>(dir)) {
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

class redrawer {
    back_buffer& buf_;

  public:
    explicit redrawer(back_buffer& buf, world& w) : buf_(buf) {

        w.reg_.on_construct<visible_in_game>()
            .connect<&redrawer::upd_visible>();
    }

    void execute(entt::registry& reg, timings::duration delta) {
        for (const auto view =
                 reg.view<const loc, const previous_sprite, const z_depth>();
             const auto entity : view) {
            const auto& [sprt] = view.get<previous_sprite>(entity);
            buf_.erase(view.get<loc>(entity), sprt,
                       view.get<z_depth>(entity).index);
            reg.remove<previous_sprite>(entity);
        }
        for (const auto view =
                 reg.view<const loc, const translation, const visible_in_game,
                          const sprite, const z_depth>();
             const auto entity : view) {
            const bool is_dead = reg.all_of<life::begin_die>(entity);
            const auto& trans = view.get<const translation>(entity);
            if (trans.is_changed() ||
                glm::all(glm::greaterThanEqual(trans.get(), loc(1))) ||
                is_dead) {
                buf_.erase(view.get<loc>(entity), view.get<sprite>(entity),
                           view.get<z_depth>(entity).index);
                if (is_dead)
                    reg.remove<visible_in_game>(entity);
            }
        }

        for (const auto view = reg.view<loc, translation, const sprite,
                                        const visible_in_game, const z_depth>();
             const auto entity : view) {
            auto& current = view.get<loc>(entity);
            auto& trans = view.get<translation>(entity);

            if (trans.is_changed() ||
                glm::all(glm::greaterThanEqual(trans.get(), loc(1)))) {
                auto& sp = view.get<sprite>(entity);
                current += trans.get();
                buf_.draw(current, sp, view.get<z_depth>(entity).index);
                trans = loc(0);
                trans.clear();
            }
        }
    }

  private:
    static void upd_visible(entt::registry& reg, const entt::entity e) {
        if (const auto ptr = reg.try_get<translation>(e))
            ptr->mark();
    }
};
