#pragma once

#include "engine/details/base_types.hpp"
#include "ecs_processors/collision.h"
#include "game/ecs_processors/drawers.h"
#include "game/ecs_processors/motion.h"
#include "libs/easing/easing.h"
#include "game/ecs_processors/collision.h"
#include "game/ecs_processors/input.h"
#include "game/ecs_processors/life.h"

#include "engine/notepad.h"
#include "engine/time.h"
#include "game/lexer.h"
#include "game/comps.h"
#include <utility>
#include <winuser.h>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>

struct coin;
struct character;

struct actor {
    static void make_base_renderable(entt::registry& reg, const entt::entity e,
                                     loc start, int z_depth_position,
                                     sprite sprt) {
        reg.emplace<visible_in_game>(e);
        reg.emplace<loc>(e, start);
        reg.emplace<translation>(e);
        reg.emplace<sprite>(e, std::move(sprt));
        reg.emplace<z_depth>(e, z_depth_position);
    }
};

inline void emplace_simple_death(entt::registry& reg, const entt::entity e) {
    reg.emplace<life::death_last_will>(e, [](entt::registry& reg_,
                                             const entt::entity self) {
        const auto dead = reg_.create();
        auto old_loc = reg_.get<loc>(self);
        actor::make_base_renderable(
            reg_, dead, old_loc - loc(1, 0), 1,
            sprite(sprite::unchecked_construct_tag{}, "___"));
        reg_.emplace<life::lifetime>(dead, std::chrono::seconds{3});
        reg_.emplace<timeline::eval_direction>(dead);
        reg_.emplace<timeline::what_do>(
            dead,
            [](entt::registry& r_, const entt::entity e_,
               timeline::direction /**/) { r_.get<translation>(e_).mark(); });
    });
}

struct projectile {
    static collision::responce on_collide(entt::registry& r,
                                          collision::self self,
                                          collision::collider other) {
        if(r.all_of<projectile>(other)) {
            return collision::responce::ignore;
        }
        r.emplace_or_replace<life::begin_die>(self);
        return collision::responce::block;
    }
    static void
    make(entt::registry& reg, entt::entity e, loc start, velocity dir,
         std::chrono::milliseconds life_time = std::chrono::seconds{1}) {
        reg.emplace<projectile>(e);
        reg.emplace<sprite>(e, sprite::unchecked_construct_tag{}, "-");
        reg.emplace<visible_in_game>(e);
        reg.emplace<z_depth>(e, 1);
        reg.emplace<loc>(e, start);
        reg.emplace<translation>(e);
        reg.emplace<non_uniform_movement_tag>(e);
        reg.emplace<velocity>(e, dir);

        reg.emplace<collision::agent>(e);

        reg.emplace<collision::on_collide>(e, &projectile::on_collide);

        reg.emplace<life::lifetime>(e, life_time);

        reg.emplace<timeline::what_do>(
            e, [duration = std::chrono::duration<double>(life_time).count(),
                call = true](entt::registry& reg_, const entt::entity e_,
                             timeline::direction) mutable {
                auto& i = reg_.get<translation>(e_).pin().y;
                static constexpr std::pair ease_range{0., 0.1};
                i = easing::easeinrange(
                    duration
                        - std::chrono::duration<double>(
                              reg_.get<life::lifetime>(e_).duration)
                              .count(),
                    // TODO(Igor) почему i = 0.1 Но прибавляется строка
                    ease_range, duration, &easing::easeInExpo);
                // TODO(Igor) why scale
                static constexpr int scale = 10;
                if(std::lround(i * scale) != 0 && call) {
                    reg_.emplace<previous_sprite>(
                        e_,
                        std::exchange(
                            reg_.get<sprite>(e_),
                            sprite(sprite::unchecked_construct_tag{}, "_")));
                    call = false;
                }
            });

        reg.emplace<timeline::eval_direction>(e, timeline::direction::forward);
        reg.emplace<life::death_last_will>(e, [](entt::registry& reg_,
                                                 const entt::entity ent) {
            const auto& current_proj_location = reg_.get<loc>(ent);
            const auto& trans = reg_.get<translation>(ent);

            const auto new_e = reg_.create();
            reg_.emplace<projectile>(new_e);
            reg_.emplace<loc>(new_e, current_proj_location);
            reg_.emplace<translation>(new_e, trans.get());
            reg_.emplace<sprite>(new_e, sprite::unchecked_construct_tag{}, "*");
            reg_.emplace<visible_in_game>(new_e);
            reg_.emplace<z_depth>(new_e, 1);
            reg_.emplace<life::lifetime>(new_e, std::chrono::seconds{1});

            reg_.emplace<life::death_last_will>(
                new_e, [](entt::registry& r_, const entt::entity ent_) {
                    auto& lb = r_.get<loc>(ent_);
                    auto& trans = r_.get<translation>(ent_);

                    auto create_fragment = [&r_, lb, trans](loc offset,
                                                            velocity dir_) {
                        const entt::entity proj = r_.create();
                        r_.emplace<projectile>(proj);
                        r_.emplace<sprite>(
                            proj, sprite::unchecked_construct_tag{}, ".");
                        r_.emplace<loc>(proj, lb + offset);
                        r_.emplace<translation>(proj, trans.get());
                        r_.emplace<visible_in_game>(proj);
                        r_.emplace<z_depth>(proj, 0);
                        r_.emplace<velocity>(proj, dir_);
                        r_.emplace<life::lifetime>(proj,
                                                   std::chrono::seconds{1});
                        r_.emplace<non_uniform_movement_tag>(proj);
                        r_.emplace<collision::agent>(proj);
                        r_.emplace<collision::on_collide>(
                            proj, &projectile::on_collide);
                    };

                    create_fragment({0., 1.}, {0, 2});
                    create_fragment(
                        {0., -1.},
                        {0., -2.}); // NOLINT(readability-magic-numbers)
                    create_fragment({0, 0}, {1, 0});
                    create_fragment({-1, 0}, {-1, 0});
                });
        });
    }
};
struct gun {
    static collision::responce on_collide(entt::registry& reg,
                                          const collision::self self,
                                          const collision::collider collider) {
        if(reg.all_of<character>(collider)) {
            reg.emplace_or_replace<life::begin_die>(self);
            return collision::responce::ignore;
        }
        return collision::responce::block;
    }

    static void make(entt::registry& reg, const entt::entity e, loc loc,
                     sprite sp) {
        reg.emplace<gun>(e);
        actor::make_base_renderable(reg, e, loc, 1, std::move(sp));
        reg.emplace<collision::agent>(e);
        reg.emplace<collision::on_collide>(e, &gun::on_collide);
    }

    static void fire(entt::registry& reg, const entt::entity character) {
        const auto& l = reg.get<loc>(character);
        auto& sh = reg.get<sprite>(character);
        auto dir = reg.get<draw_direction>(character);

        const auto proj = reg.create();
        using namespace pos_declaration;
        const loc spawn_translation =
            dir == draw_direction::right ? loc(sh.bounds()[X], 0) : loc(-1, 0);
        static constexpr float projectile_start_force = 15.;
        projectile::make(
            reg, proj, l + spawn_translation,
            velocity(projectile_start_force * static_cast<float>(dir), 0),
            std::chrono::seconds{4});
    }
};

namespace timeline {
static void
make(entt::registry& reg, const entt::entity e,
     std::function<void(entt::registry&, entt::entity, direction)>
         what_do // NOLINT(performance-unnecessary-value-param)
     ,
     const std::chrono::milliseconds duration = std::chrono::seconds{1},
     direction dir = direction::forward) {
    reg.emplace<life::lifetime>(e, duration);
    reg.emplace<timeline::what_do>(e, std::move(what_do));
    reg.emplace<timeline::eval_direction>(e, dir);
}
}; // namespace timeline

// waits for the end of time and call a given function
struct timer {
    static void make(entt::registry& reg, entt::entity e,
                     std::function<void(entt::registry&, entt::entity)> what_do,
                     std::chrono::milliseconds duration = std::chrono::seconds{
                         1}) {
        reg.emplace<life::lifetime>(e, duration);
        reg.emplace<life::death_last_will>(e, std::move(what_do));
    }
};

struct character {
    static collision::responce on_collide(entt::registry& reg,
                                          const collision::self self,
                                          const collision::collider collider) {
        // TODO(Igor) collision functions without cast?
        if(reg.all_of<gun>(collider)) {
            auto& animation = reg.get<sprite>(self);
            auto dir = reg.get<draw_direction>(self);
            reg.emplace<previous_sprite>(
                self, std::exchange(
                          animation,
                          sprite(sprite::unchecked_construct_tag{},
                                 dir == draw_direction::left ? "-#" : "#-")));
            reg.emplace<on_change_direction>(
                self, [self, &reg](draw_direction new_dr, sprite& sp) {
                    const auto* s =
                        new_dr == draw_direction::left ? "-#" : "#-";
                    reg.emplace<previous_sprite>(
                        self,
                        std::exchange(
                            sp, sprite(sprite::unchecked_construct_tag{}, s)));
                });

            auto& [signal] = reg.get<input::processor::down_signal>(self);
            signal.connect([](entt::registry& reg_, const entt::entity chrcter,
                              const input::state_t& state,
                              const timings::duration /*dt*/) {
                static constexpr int every_key = 5;
                if(auto key = input::has_key(state, input::key::space);
                   key && !(key->press_count % every_key)) {
                    gun::fire(reg_, chrcter);
                }
            });
            return collision::responce::ignore;
        }
        return collision::responce::block;
    }

    static void make(entt::registry& reg, const entt::entity e, loc l) {
        reg.emplace<character>(e);
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

    template<input::key UP = input::key::w, input::key LEFT = input::key::a,
             input::key DOWN = input::key::s, input::key RIGHT = input::key::d>
    static void process_movement_input(entt::registry& reg,
                                       const entt::entity e,
                                       const input::state_t& state,
                                       const timings::duration /*dt*/) {
        auto& vel = reg.get<velocity>(e);
        static constexpr glm::vec2 acceleration_range{0.8, 1.1};
        auto upd = [](double& vel, int value, int pressed_count) mutable {
            vel += value
                   * easing::easeInExpo(glm::mix(acceleration_range.x,
                                                 acceleration_range.y,
                                                 pressed_count / 100.));
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
};

enum class game_status_flag {
    unset,
    find,
    kill,
};

namespace kitten {
struct kitten_tag {};
inline collision::responce on_collide(entt::registry& reg, collision::self self,
                                      collision::collider collider,
                                      game_status_flag& game_over_flag) {
    if(reg.all_of<projectile>(collider)) {
        game_over_flag = game_status_flag::kill;
        emplace_simple_death(reg, self);
        reg.emplace<life::begin_die>(self);
    }

    if(reg.all_of<character>(collider)) {
        reg.emplace<life::begin_die>(collider);
        reg.emplace<life::begin_die>(self);
        auto w = static_control{};
        auto w_uuid = w.get_id();
        notepad::push_command(std::move(
            [w = std::move(w)](notepad* np, scintilla* /*sct*/) mutable {
                static constexpr pos w_size = pos(20, 50);
                static constexpr pos w_pos = pos(20, 0);
                HDC hDC = GetDC(w);
                RECT r = { 0, 0, 0, 0 };
                DrawText(hDC, w.text.data(), static_cast<int>(w.text.size()), &r, DT_CALCRECT);
                ReleaseDC(w, hDC);
                w.with_size(w_size) // {r.right-r.left, r.bottom-r.top}
                    .with_position(w_pos) // {r.left, r.top}
                    .with_text("#")
                    .with_text_color(RGB(0, 0, 0))
                    .show(np);
                np->static_controls.emplace_back(std::move(w));
            }));
        const auto end_anim = reg.create();
        reg.emplace<timeline::eval_direction>(end_anim);
        reg.emplace<timeline::what_do>(
            end_anim, [w_uuid = w_uuid, timer = 0](
                          entt::registry& /*r*/, const entt::entity /*e*/,
                          timeline::direction /**/) mutable {
                timer += 1;
                // NOLINTNEXTLINE(readability-magic-numbers)
                if(!(timer % 2)) {
                    notepad::push_command([w_uuid](notepad* np,
                                                   scintilla* /*sct*/) {
                        auto w = std::ranges::find_if(
                            np->static_controls,
                            [w_uuid](auto& w) { return w.get_id() == w_uuid; });
                        if(w == np->static_controls.end()) {
                            return;
                        }
                        w->position.x = static_cast<int>(easing::easeinrange(
                            // NOLINTNEXTLINE(readability-magic-numbers)
                            0.1,
                            // NOLINTNEXTLINE(readability-magic-numbers)
                            {20, 300}, 4, &easing::easeInExpo));
                        std::cout <<  w->position.x << '\n';
                        SetWindowText(*w, w->text.data());
                        SetWindowPos(*w, HWND_TOP, w->position.x, w->position.y,
                                     w->size.x, w->size.y, 0);
                    });
                }
            });
        using namespace std::chrono_literals;
        timer::make(
            reg, end_anim,
            [w_uuid = w_uuid, &game_over_flag](entt::registry& /*reg*/,
                                               const entt::entity /*timer*/) {
                notepad::push_command(
                    [w_uuid](notepad* np, scintilla* /*sct*/) {
                        auto it = std::ranges::find_if(
                            np->static_controls,
                            [w_uuid](auto& w) { return w.get_id() == w_uuid; });
                        np->static_controls.erase(it);
                    });

                game_over_flag = game_status_flag::find;
            },
            4s);
    }
    return collision::responce::ignore;
};
inline void make(entt::registry& reg, const entt::entity e, loc loc, sprite sp,
                 game_status_flag& game_over_flag) {
    reg.emplace<kitten_tag>(e);
    actor::make_base_renderable(reg, e, loc, 1, std::move(sp));
    reg.emplace<collision::agent>(e);
    reg.emplace<collision::on_collide>(
        e, collision::on_collide{[&game_over_flag](
                                     entt::registry& reg, collision::self self,
                                     collision::collider collider) {
            return kitten::on_collide(reg, self, collider, game_over_flag);
        }});
}

} // namespace kitten

struct atmosphere {
    struct color_range {
        COLORREF start{RGB(0, 0, 0)};
        COLORREF end{RGB(255, 255, 255)};
    };
    static void make(entt::registry& reg, const entt::entity e) {
        timer::make(reg, e, &atmosphere::run_cycle, time_between_cycle);
        reg.emplace<timeline::eval_direction>(e, timeline::direction::reverse);
    }
    static void run_cycle(entt::registry& reg, const entt::entity timer) {
        const auto cycle_timeline = reg.create();

        timeline::make(reg, cycle_timeline, &atmosphere::update_cycle,
                       cycle_duration,
                       reg.get<timeline::eval_direction>(timer).value);

        reg.emplace<color_range>(cycle_timeline);
        reg.emplace<life::death_last_will>(
            cycle_timeline,
            [](entt::registry& reg_, const entt::entity cycle_timeline_) {
                const auto again_timer = reg_.create();
                timer::make(reg_, again_timer, &atmosphere::run_cycle,
                            time_between_cycle);
                reg_.emplace<timeline::eval_direction>(
                    again_timer,
                    timeline::invert(
                        reg_.get<timeline::eval_direction>(cycle_timeline_)
                            .value));
            });
    }
    static void update_cycle(entt::registry& reg, const entt::entity e,
                             const timeline::direction d) {
        const auto& [start, end] = reg.get<color_range>(e);
        const auto& [current_lifetime] = reg.get<life::lifetime>(e);

        double value = (current_lifetime - cycle_duration)
                       / (std::chrono::duration<double>{0} - cycle_duration);
        value *= static_cast<double>(d);

        const COLORREF new_back_color =
            RGB(std::lerp(GetRValue(start), GetRValue(end), value),
                std::lerp(GetGValue(start), GetGValue(end), value),
                std::lerp(GetBValue(start), GetBValue(end), value));

        const COLORREF new_front_color =
            RGB(std::lerp(GetRValue(start), GetRValue(end), -value),
                std::lerp(GetGValue(start), GetGValue(end), -value),
                std::lerp(GetBValue(start), GetBValue(end), -value));

        notepad::push_command(
            [new_back_color, new_front_color](notepad* np, scintilla* sct) {
                np->back_color = new_back_color;
                for(auto& w: np->static_controls) {
                    w.fore_color = new_front_color;
                    SetWindowText(w.get_wnd(), w.text.data());
                }
                // TODO(Igor) values from lexer
                static constexpr int space_code = 32;
                static constexpr int style_start = 100;
                static_assert(static_cast<int>(' ') == space_code);
                int style = style_start + space_code;
                // TODO(Igor) to Scintilla wrapper
                sct->dcall2(SCI_STYLESETBACK, STYLE_DEFAULT, new_back_color);
                sct->dcall2(SCI_STYLESETBACK, 0, new_back_color);
                sct->dcall2(SCI_STYLESETFORE, STYLE_DEFAULT, new_front_color);
                sct->dcall2(SCI_STYLESETFORE, 0, new_front_color);
                for(auto i: ALL_COLORS) {
                    sct->dcall2(SCI_STYLESETBACK, style, new_back_color);
                    style++;
                }
            });
    }

private:
    inline static const auto time_between_cycle = std::chrono::seconds{20};
    inline static const auto cycle_duration = std::chrono::seconds{2};
};
