#pragma once

#include <utility>
#include <array>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include "glm/gtx/easing.hpp"

#include "Windows.h"
#include <winuser.h>

#include "engine/details/base_types.hpp"
#include "game/ecs_processors/collision.h"
#include "game/ecs_processors/drawers.h"
#include "game/ecs_processors/motion.h"
#include "game/ecs_processors/input.h"
#include "game/ecs_processors/life.h"
#include "game/lexer.h"
#include "game/comps.h"
#include "engine/notepad.h"
#include "engine/time.h"
#include "game/lexer.h"
#include "lexer.h"

struct character_tag;


namespace factories {
struct actor_tag {}
    inline void make_base_renderable(entt::registry& reg, const entt::entity e,
                                     loc start, int z_depth_position,
                                     sprite sprt) {
        reg.emplace<visible_in_game>(e);
        reg.emplace<loc>(e, start);
        reg.emplace<translation>(e);
        reg.emplace<sprite>(e, std::move(sprt));
        reg.emplace<z_depth>(e, z_depth_position);
    }

inline void emplace_simple_death_anim(entt::registry& reg, const entt::entity e) {
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
}
namespace projectile {
using rng_type = std::mt19937;
static rng_type rng{};
struct projectile_tag {}

    inline collision::responce on_collide(entt::registry& r,
                                          collision::self self,
                                          collision::collider other) {
        if(r.all_of<projectile_tag>(other)) {
            return collision::responce::ignore;
        }
        r.emplace_or_replace<life::begin_die>(self);
        return collision::responce::block;
    }
    inline void
    make(entt::registry& reg, entt::entity e, loc start, velocity dir,
         std::chrono::milliseconds life_time = std::chrono::seconds{1}) {
        reg.emplace<projectile_tag>(e);
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

        // NOLINTBEGIN(readability-magic-numbers)
        static std::uniform_int_distribution<projectile1::rng_type::result_type>
            fall_dist(3, 7);
        reg.emplace<timeline::what_do>(
            e, [total_iterations = static_cast<double>((life_time / timings::dt)) * 1.,
                current_iteration = 0., start_y = start.y,
                changing_y = fall_dist(projectile1::rng),
                call = true](entt::registry& reg_, const entt::entity e_,
                             timeline::direction) mutable {
                using namespace std::chrono_literals;
                auto& tr = reg_.get<translation>(e_).pin().y;
                // NOLINTBEGIN( bugprone-narrowing-conversions)
                auto l = reg_.get<loc>(e_);
                double y = changing_y
                               * glm::backEaseIn(
                                   std::min(total_iterations, current_iteration)
                                   / total_iterations)
                           + start_y;
                tr = glm::round(y - l.y);
                if(current_iteration / total_iterations > 0.6 && call) {
                    reg_.emplace<previous_sprite>(
                        e_,
                        std::exchange(
                            reg_.get<sprite>(e_),
                            sprite(sprite::unchecked_construct_tag{}, "_")));
                    call = false;
                    // NOLINTEND( bugprone-narrowing-conversions)
                }
                current_iteration += 1.;
                // NOLINTEND(readability-magic-numbers)
            });

        reg.emplace<timeline::eval_direction>(e, timeline::direction::forward);
        reg.emplace<life::death_last_will>(e, [](entt::registry& reg_,
                                                 const entt::entity ent) {
            const auto& current_proj_location = reg_.get<loc>(ent);
            const auto& trans = reg_.get<translation>(ent);

            const auto new_e = reg_.create();
            reg_.emplace<projectile_tag>(new_e);
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
                        r_.emplace<projectile_tag>(proj);
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
namespace gun {
    struct gun_tag{};
    inline collision::responce on_collide(entt::registry& reg,
                                          const collision::self self,
                                          const collision::collider collider) {
        if(reg.all_of<character_tag>(collider)) {
            reg.emplace_or_replace<life::begin_die>(self);
            return collision::responce::ignore;
        }
        return collision::responce::block;
    }

    inline void make(entt::registry& reg, const entt::entity e, loc loc,
                     sprite sp) {
        reg.emplace<gun_tag>(e);
        actor::make_base_renderable(reg, e, loc, 1, std::move(sp));
        reg.emplace<collision::agent>(e);
        reg.emplace<collision::on_collide>(e, &gun::on_collide);
    }

    inline void fire(entt::registry& reg, const entt::entity character) {
        const auto& l = reg.get<loc>(character);
        auto& sh = reg.get<sprite>(character);
        auto dir = reg.get<draw_direction>(character);

        const auto proj = reg.create();
        const loc spawn_translation =
            dir == draw_direction::right ? loc(sh.bounds().x, 0) : loc(-1, 0);
        static constexpr float projectile_start_force = 15.;
        static constexpr pos random_end_y{3, 5};
        static std::uniform_int_distribution<projectile::rng_type::result_type>
            life_dist(random_end_y.x, random_end_y.y);
        projectile::make(
            reg, proj, l + spawn_translation,
            velocity(projectile_start_force * static_cast<float>(dir), 0),
            std::chrono::seconds{life_dist(projectile::rng)});
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
namespace timer {
    struct timer_tag{};
    inline void make(entt::registry& reg, entt::entity e,
                     std::function<void(entt::registry&, entt::entity)> what_do,
                     std::chrono::milliseconds duration = std::chrono::seconds{
                         1}) {
        reg.emplace<life::lifetime>(e, duration);
        reg.emplace<life::death_last_will>(e, std::move(what_do));
    }
};

namespace character {
    struct character_tag{};
    inline collision::responce on_collide(entt::registry& reg,
                                          const collision::self self,
                                          const collision::collider collider) {
        // TODO(Igor) collision functions without cast?
        if(reg.all_of<gun_tag>(collider)) {
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

    inline void make(entt::registry& reg, const entt::entity e, loc l) {
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

    template<input::key UP = input::key::w, input::key LEFT = input::key::a,
             input::key DOWN = input::key::s, input::key RIGHT = input::key::d>
    void process_movement_input(entt::registry& reg,
                                       const entt::entity e,
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
};

enum class game_status_flag {
    unset,
    find,
    kill,
};

namespace game_over {
    static_control make_character(){
        return static_control{}
          .with_position(pos(20, 0))
          .with_text("#")
          .with_text_color(RGB(0, 0, 0));
    }
    void push_controls(std::array<static_control, 2>&& ctrls){
        notepad::push_command(
            [ctrls = std::move(ctrls)](
                          notepad* np, scintilla* /*sct*/) mutable {
                // hide all other static controls
                for(auto& i: np->static_controls) {
                    i.text = "";
                    SetWindowText(i.get_wnd(), i.text.data());
                }
                static constexpr pos w_size = pos(20, 50);
                for(auto& i : ctrls):
                    // TODO(Igor): Shrink size
                    i.with_size(w_size) // {r.right-r.left, r.bottom-r.top}
                        .show(np);
                    np->static_controls.emplace_back(std::move(ch));
                }
            });
    }
    void bad_end_animation(static_control::id_t char_wnd_id, 
        entt::registry& reg, entt::entity end_anim){
        reg.emplace<timeline::eval_direction>(end_anim);
        reg.emplace<timeline::what_do>(
            end_anim, [ch_uuid = char_wnd_id,
                total_iterations=static_cast<double>(4s/timings::dt)*1.,
                current_iteration = 0.,
                start_x = 20,
                changing_x = 250
                ](
                          entt::registry& /*r*/, const entt::entity /*e*/,
                          timeline::direction /**/) mutable {
                double alpha = std::min(total_iterations, current_iteration)/total_iterations;
                double ch_x = changing_x * glm::backEaseOut(alpha) + start_x;
                current_iteration += 1.;
                notepad::push_command([ch_uuid, ch_x](
                                          notepad* np, scintilla* /*sct*/) {
                    auto ch = std::ranges::find_if(
                        np->static_controls,
                        [ch_uuid](auto& w) { return w.get_id() == ch_uuid; });
                        if(ch == np->static_controls.end()
                       ) {
                        return;
                    }
                    ch->position.x =  static_cast<int32_t>(glm::round(ch_x));
                    SetWindowText(*ch, ch->text.data());
                    SetWindowPos(*ch, HWND_TOP, ch->position.x, ch->position.y,
                                 ch->size.x, ch->size.y, 0);
                    // NOLINTEND(readability-magic-numbers)
                });
            });
    }

    void good_end_animation(static_control::id_t char_wnd_id, static_control::id_t kitten_wnd_id, 
    entt::registry& reg, entt::entity end_anim){
        using namespace std::chrono_literals;
        reg.emplace<timeline::eval_direction>(end_anim);
        reg.emplace<timeline::what_do>(
            end_anim, [ch_uuid =  char_wnd_id, k_uuid = kitten_wnd_id, 
                total_iterations=static_cast<double>(4s/timings::dt)*1.,
                current_iteration = 0.,
                start_x = 20,
                changing_x = 280
                ](
                          entt::registry& /*r*/, const entt::entity /*e*/,
                          timeline::direction /**/) mutable {
                double alpha = std::min(total_iterations, current_iteration)/total_iterations;
                double ch_x = changing_x * glm::quarticEaseOut(alpha) + start_x;
                double k_x  = -1 * (changing_x-30) * glm::cubicEaseOut(alpha) + start_x+280;

                current_iteration += 1.;
                notepad::push_command([ch_uuid, k_uuid, k_x, ch_x](
                                          notepad* np, scintilla* /*sct*/) {
                    auto ch = std::ranges::find_if(
                        np->static_controls,
                        [ch_uuid](auto& w) { return w.get_id() == ch_uuid; });
                    auto k = std::ranges::find_if(
                        np->static_controls,
                        [k_uuid](auto& w) { return w.get_id() == k_uuid; });
                    if(ch == np->static_controls.end()
                       || k == np->static_controls.end()) {
                        return;
                    }
                    ch->position.x =  static_cast<int32_t>(glm::round(ch_x));
                    k->position.x = static_cast<int32_t>(glm::round(k_x));
                    SetWindowText(*ch, ch->text.data());
                    SetWindowPos(*ch, HWND_TOP, ch->position.x, ch->position.y,
                                 ch->size.x, ch->size.y, 0);
                    SetWindowText(*k, k->text.data());
                    SetWindowPos(*k, HWND_TOP, k->position.x, k->position.y,
                                 k->size.x, k->size.y, 0);
                    // NOLINTEND(readability-magic-numbers)
                });
            });


    }

    void create_input_wait(entt::registry reg, game_status_flag status, game_status_flag& game_over_flag){
        auto& [input_callback] =
            reg.emplace<input::processor::down_signal>(ent);
        input_callback.connect([&game_over_flag, status](entt::registry& reg,
               const entt::entity e,
               const input::state_t& state,
               const timings::duration /*dt*/){
            notepad::push_command(
                [](notepad* np, scintilla* /*sct*/) {
                    np->static_controls->clear();
            });
            game_over_flag = status;
        });
        notepad::push_command([](notepad* np, scintilla* /*sct*/) {
                np->static_controls.emplace_back(static_control{}
                            .with_position(pos(350, 0))
                            .with_text("Press any key to Restart")
                            .with_text_color(RGB(0, 0, 0))
                            .show(np)
            );
       });


    }


}


namespace kitten {
struct kitten_tag {};
inline collision::responce on_collide(entt::registry& reg, collision::self self,
                                      collision::collider collider,
                                      game_status_flag& game_over_flag) {
    if(reg.all_of<projectile_tag>(collider)) {
        factories::emplace_simple_death_anim(reg, self);
        reg.emplace<life::begin_die>(self);
        reg.emplace<life::begin_die>(collider);
        auto char_wnd = make_character();
        auto ch_uuid = char_wnd.get_id();
        game_over::push_controls(std::make_array({
             std::move(char_wnd),
             // kitten
             static_control{}
                .with_position(pos(50, 0))
                .with_text("___")
                .with_text_color(RGB(0, 0, 0))

        }));
        const auto end_anim = reg.create();
        game_over::bad_end_animation(ch_uuid, reg, end_anim);
        using namespace std::chrono_literals;
        timer::make(
                reg, end_anim, [&game_over_flag](entt::registry& reg,
                                                    entt::entity /*timer*/){
                game_over::create_input_wait(reg, game_status_flag::find, game_over_flag);
                }, 5s);
    }
    if(reg.all_of<character_tag>(collider)) {
        reg.emplace<life::begin_die>(self);
        reg.emplace<life::begin_die>(collider);
        auto char_wnd = make_character();
        auto ch_uuid = char_wnd.get_id();
        auto kitten_wnd = static_control{}
                            .with_position(pos(300 + 280, 0))
                            .with_text(reg.get<sprite>(end_anim).data())
                            .with_text_color(RGB(0, 0, 0));
        auto k_uuid = kitten_wnd.get_id();
        game_over::push_controls(std::make_array({
             std::move(char_wnd),
             std::move(kitten_wnd)
        }));
        const auto end_anim = reg.create();
        reg.emplace<sprite>(end_anim, reg.get<sprite>(self));
        game_over::good_end_animation(ch_uuid, k_uuid, reg, end_anim);
        using namespace std::chrono_literals;
        timer::make(
                reg, end_anim, [&game_over_flag](entt::registry& reg,
                                                    entt::entity /*timer*/){
                game_over::create_input_wait(reg, game_status_flag::kill,  game_over_flag);
                }, 5s);
    }
    return collision::responce::ignore;
};

inline void make(entt::registry& reg, entt::entity e, loc loc, sprite sp,
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

namespace atmosphere {
    struct atmospere_tag{};
    struct color_range {
        COLORREF start{RGB(0, 0, 0)};
        COLORREF end{RGB(255, 255, 255)};
    };

    inline static const auto time_between_cycle = std::chrono::seconds{20};
    inline static const auto cycle_duration = std::chrono::seconds{2};

    inline void make(entt::registry& reg,  entt::entity e) {
        timer::make(reg, e, &atmosphere::run_cycle, time_between_cycle);
        reg.emplace<timeline::eval_direction>(e, timeline::direction::reverse);
    }
    inline void run_cycle(entt::registry& reg, entt::entity timer) {
        const auto cycle_timeline = reg.create();

        timeline::make(reg, cycle_timeline, &atmosphere::update_cycle,
                       cycle_duration,
                       reg.get<timeline::eval_direction>(timer).value);

        reg.emplace<color_range>(cycle_timeline);
        reg.emplace<life::death_last_will>(
            cycle_timeline,
            [](entt::registry& reg_, entt::entity cycle_timeline_) {
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
    inline void update_cycle(entt::registry& reg, entt::entity e,
                             timeline::direction d) {
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
                int style = MY_STYLE_START + PRINTABLE_RANGE.first;
                for(auto i : {0, STYLE_DEFAULT}){
                    sct->set_back_color(i, new_back_color);
                    sct->set_text_color(i, new_front_color);
                }
                for(size_t style = MY_STYLE_START + PRINTABLE_RANGE.first; 
                        style < ALL_COLORS.size(); style++) { 
                    sct->set_back_color(style, new_back_color); 
                }
            });
    }

};
