#include <array>
#include <condition_variable>
#include <utility>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>

#include "Windows.h"
#include <winuser.h>

#include "engine/details/base_types.hpp"
#include "engine/notepad.h"
#include "game/factories.h"
#include "game/lexer.h"
#include "game/systems/drawers.h"
#include "game/systems/input.h"

struct total_iterations {
    double v;
};
struct current_iteration {
    double v;
};
struct changing_value {
    double v;
};
struct start_value {
    double v;
};
namespace factories {
void emplace_simple_death_anim(entt::handle h) {
    h.emplace<life::dying_wish>(
        life::dying_wish::function_type{+[](const void*, entt::handle dying) {
            const auto dead =
                entt::handle{*dying.registry(), dying.registry()->create()};
            factories::make_base_renderable(
                dead, dying.get<loc>() - loc(1, 0), 1,
                sprite(sprite::unchecked_construct_tag{}, "___"));
            dead.emplace<life::life_time>(std::chrono::seconds{3});
            dead.emplace<timeline::eval_direction>();
            dead.emplace<timeline::what_do>(
                +[](const void*, entt::handle h, timeline::direction,
                    timings::duration) { h.get<translation>().mark(); });
        }});
}

} // namespace factories
namespace projectile {
namespace {} // namespace
void on_collide(const void* /**/, entt::registry& r, collision::self self,
                collision::collider other) {
    if(!r.all_of<projectile_tag>(other)) {
        r.emplace_or_replace<life::begin_die>(self);
    }
}
template<typename Id>
struct projectile_sprite {
    entt::entity where = entt::null;
};
struct base_sprite_tag {};
struct death_sprite_tag {};
struct explosion_sprite_tag {};
struct fragment_sprite_tag {};
void initialize_for_all(entt::registry& reg) {
    auto base_sprite = reg.create();
    reg.emplace<sprite>(base_sprite, sprite::unchecked_construct_tag{}, "-");
    auto death_sprite = reg.create();
    reg.emplace<sprite>(death_sprite, sprite::unchecked_construct_tag{}, "_");
    auto explosion_sprite = reg.create();
    reg.emplace<sprite>(explosion_sprite, sprite::unchecked_construct_tag{},
                        "*");
    auto fragment_sprite = reg.create();
    reg.emplace<sprite>(fragment_sprite, sprite::unchecked_construct_tag{},
                        ".");
    reg.ctx().emplace<projectile_sprite<base_sprite_tag>>(base_sprite);
    reg.ctx().emplace<projectile_sprite<death_sprite_tag>>(death_sprite);
    reg.ctx().emplace<projectile_sprite<explosion_sprite_tag>>(
        explosion_sprite);
    reg.ctx().emplace<projectile_sprite<fragment_sprite_tag>>(fragment_sprite);
}

void make(entt::handle h, loc start, velocity dir,
          std::chrono::milliseconds life_time) {
    auto sprt_i =
        h.registry()->ctx().get<projectile_sprite<base_sprite_tag>>().where;
    h.emplace<projectile_tag>();
    h.emplace<visible_tag>();
    h.emplace<current_rendering_sprite>(sprt_i);
    h.emplace<z_depth>(1);
    h.emplace<loc>(start);
    h.emplace<translation>();
    h.emplace<non_uniform_movement_tag>();
    h.emplace<velocity>(dir);
    h.emplace<collision::agent>();
    h.emplace<collision::hit_extends>(
        h.registry()->get<sprite>(sprt_i).bounds());
    h.emplace<collision::responce_func>(
        collision::responce_func::function_type{&projectile::on_collide});
    h.emplace<life::life_time>(life_time);

    static std::uniform_int_distribution<> fall_dist(3, 7);

    h.emplace<total_iterations>(static_cast<double>((life_time / timings::dt))
                                * 1.);
    h.emplace<current_iteration>(0.);
    h.emplace<changing_value>(
        static_cast<double>(fall_dist(h.registry()->ctx().get<random_t>())));
    h.emplace<start_value>(start.y);

    h.emplace<timeline::eval_direction>(timeline::direction::forward);
    h.emplace<timeline::what_do>(+[](const void*, entt::handle h,
                                     timeline::direction, timings::duration) {
        using namespace std::chrono_literals;
        auto& tr = h.get<translation>().pin().y;
        auto changing_y = h.get<const changing_value>().v;
        auto total_iters = h.get<const total_iterations>().v;
        auto& curr_iter = h.get<current_iteration>().v;
        auto start_y = h.get<const start_value>().v;
        auto l = h.get<loc>();
        double y = changing_y
                       * glm::backEaseIn(std::min(total_iters, curr_iter)
                                         / total_iters)
                   + start_y;
        tr = glm::round(y - l.y);
        curr_iter += 1.;
    });

    struct projectile_i {
        entt::entity where = entt::null;
    };
    auto sprite_timer = entt::handle{*h.registry(), h.registry()->create()};
    sprite_timer.emplace<projectile_i>(h.entity());
    timer::make(
        sprite_timer,
        life::dying_wish::function_type{+[](const void*, entt::handle timer) {
            auto proj = timer.get<projectile_i>().where;
            // TODO(Igor): RAII
            if(timer.registry()->valid(proj)) {
                timer.registry()->emplace<previous_sprite>(
                    proj, std::exchange(
                              timer.registry()
                                  ->get<current_rendering_sprite>(proj)
                                  .where,
                              timer.registry()
                                  ->ctx()
                                  .get<projectile_sprite<death_sprite_tag>>()
                                  .where));
            }
        }},
        life_time / 100 * 60);

    h.emplace<life::dying_wish>(life::dying_wish::function_type{+[](const void*,
                                                                    entt::handle
                                                                        h) {
        const auto& current_proj_location = h.get<loc>();
        const auto& trans = h.get<translation>();
        const auto bang = entt::handle{*h.registry(), h.registry()->create()};
        bang.emplace<projectile_tag>();
        bang.emplace<loc>(current_proj_location);
        bang.emplace<translation>(trans.get());
        bang.emplace<current_rendering_sprite>(
            h.registry()
                ->ctx()
                .get<projectile_sprite<explosion_sprite_tag>>()
                .where);
        bang.emplace<visible_tag>();
        bang.emplace<z_depth>(1);
        bang.emplace<life::life_time>(std::chrono::seconds{1});

        bang.emplace<life::dying_wish>(
            life::dying_wish::function_type{[](const void*, entt::handle h) {
                auto& lb = h.get<loc>();
                auto& trans = h.get<translation>();

                auto create_fragment =
                    [&reg = *h.registry(), lb, trans,
                     sprt = h.registry()
                                ->ctx()
                                .get<projectile_sprite<fragment_sprite_tag>>()
                                .where](loc offset, velocity dir_) {
                        const auto frag = entt::handle{reg, reg.create()};
                        frag.emplace<projectile_tag>();
                        frag.emplace<current_rendering_sprite>(sprt);
                        frag.emplace<loc>(lb + offset);
                        frag.emplace<translation>(trans.get());
                        frag.emplace<visible_tag>();
                        frag.emplace<z_depth>(0);
                        frag.emplace<velocity>(dir_);
                        frag.emplace<life::life_time>(std::chrono::seconds{1});
                        frag.emplace<non_uniform_movement_tag>();
                        frag.emplace<collision::hit_extends>(
                            frag.registry()->get<sprite>(sprt).bounds());
                        frag.emplace<collision::agent>();
                        frag.emplace<collision::responce_func>(
                            collision::responce_func::function_type{
                                &projectile::on_collide});
                    };
                create_fragment(loc{0., 1.}, velocity{{0, 2}});
                create_fragment(loc{0., -1.}, velocity{{0., -2.}});
                create_fragment(loc{0, 0}, velocity{{1, 0}});
                create_fragment(loc{-1, 0}, velocity{{-1, 0}});
            }});
    }});
}
} // namespace projectile

namespace gun {
void on_collide(const void* /*payload*/, entt::registry& reg,
                const collision::self self,
                const collision::collider collider) {
    if(reg.all_of<character::character_tag>(collider)) {
        reg.emplace_or_replace<life::begin_die>(self);
        auto msg_wnd_uuid = reg.ctx().get<static_control_handler::id_t>(
            entt::hashed_string{"msg_window"});
        notepad::push_command([msg_wnd_uuid](notepad* np, scintilla*) {
            auto w = std::ranges::find_if(
                np->static_controls,
                [msg_wnd_uuid](auto& w) { return w.get_id() == msg_wnd_uuid; });
            static const char* msg = "Wow! It's a little gun!";
            SetWindowText(*w, msg);
            w->text = msg;
        });
    }
}

void make(entt::handle h, loc loc, sprite sp) {
    h.emplace<gun_tag>();
    h.emplace<collision::hit_extends>(sp.bounds());
    factories::make_base_renderable(h, loc, 1, std::move(sp));
    h.emplace<collision::agent>();
    h.emplace<collision::responce_func>(&gun::on_collide);
}

void fire(entt::handle character) {
    const auto& l = character.get<loc>();
    auto hit_box = character.get<collision::hit_extends>();
    auto dir = character.get<draw_direction>();

    const auto proj =
        entt::handle{*character.registry(), character.registry()->create()};
    const loc spawn_translation =
        dir == draw_direction::right ? loc(hit_box.v.x, 0) : loc(-1, 0);
    constexpr pos random_end_y{3, 5};
    constexpr pos random_velocity_range{15, 25};
    std::uniform_int_distribution<> life_dist(random_end_y.x, random_end_y.y);
    std::uniform_int_distribution<> velocity_dist(random_velocity_range.x,
                                                  random_velocity_range.y);
    auto& rnd = proj.registry()->ctx().get<random_t>();
    projectile::make(
        proj, l + spawn_translation,
        velocity(loc{velocity_dist(rnd) * static_cast<double>(dir), 0}),
        std::chrono::seconds{life_dist(rnd)});
}
} // namespace gun

namespace character {

void on_collide(const void* /*payload*/, entt::registry& reg,
                const collision::self self,
                const collision::collider collider) {
    auto& trans = reg.get<translation>(self);
    trans.pin() = pos(0);
    trans.clear();
    if(reg.all_of<gun::gun_tag>(collider)) {
        auto dir = reg.get<draw_direction>(self);
        auto new_left_sprt = reg.create();
        auto new_sprt = sprite(sprite::unchecked_construct_tag{}, "-#");
        reg.get<collision::hit_extends>(self).v = pos(2, 1);
        reg.emplace<sprite>(new_left_sprt, std::move(new_sprt));
        auto new_right_sprt = reg.create();
        reg.emplace<sprite>(new_right_sprt,
                            sprite(sprite::unchecked_construct_tag{}, "#-"));
        auto old_sprt = std::exchange(
            reg.get<current_rendering_sprite>(self).where,
            dir == draw_direction::left ? new_left_sprt : new_right_sprt);
        reg.emplace<direction_sprite<draw_direction::left>>(self,
                                                            new_left_sprt);
        reg.emplace<direction_sprite<draw_direction::right>>(self,
                                                             new_right_sprt);
        // TODO(Igor): RAII for sprites
        reg.emplace<previous_sprite>(self, old_sprt);

        auto additional_input = reg.create();
        reg.emplace<input::task_owner>(additional_input, self);
        reg.emplace<input::key_down_task>(
            additional_input,
            input::key_down_task::function_type{
                +[](const void*, entt::handle additional_input,
                    std::span<input::key_state> state) {
                    static constexpr int every_key = 5;

                    if(auto key = input::has_key(state, input::key::space);
                       key && !(key->press_count % every_key)) {
                        gun::fire(
                            {*additional_input.registry(),
                             additional_input.get<input::task_owner>().v});
                    }
                }});
    }
}
} // namespace character

namespace game_over {

static_control_handler make_character() {
    static constexpr pos start_pos{20, 0};
    static constexpr pos w_size = pos(20, 100);
    return static_control_handler{}
        .with_position(start_pos)
        .with_text("#")
        .with_text_color(RGB(0, 0, 0))
        .with_size(w_size);
}
void push_controls(std::array<static_control_handler, 2>&& ctrls) {
    notepad::push_command(
        [ctrls = std::move(ctrls)](notepad* np, scintilla* sct) mutable {
            // hide all other static controls
            for(auto& i: np->static_controls) {
                i.text = "";
                SetWindowText(i.get_wnd(), i.text.data());
            }
            for(auto& i: ctrls) {
                // TODO(Igor): Shrink size
                // i // {r.right-r.left, r.bottom-r.top}
                // FIXME(Igor): implicit
                if(i.fore_color == RGB(0, 0, 0)) {
                    i.fore_color = sct->get_text_color(STYLE_DEFAULT);
                }
                np->show_static_control(std::move(i));
            }
        });
}

void bad_end_animation(entt::handle end_anim) {
    end_anim.emplace<timeline::eval_direction>();
    using namespace std::chrono_literals;
    static constexpr double start_x = 20.;
    static constexpr double x_duration = 200.;

    end_anim.emplace<total_iterations>(static_cast<double>(4s / timings::dt)
                                       * 1.);

    end_anim.emplace<current_iteration>(0.);
    end_anim.emplace<changing_value>(x_duration);
    end_anim.emplace<start_value>(start_x);
    end_anim.emplace<timeline::what_do>(+[](const void*, entt::handle h,
                                            timeline::direction,
                                            timings::duration) {
        auto& cur_iter = h.get<current_iteration>().v;
        auto total_iter = h.get<total_iterations>().v;
        double alpha = std::min(total_iter, cur_iter) / total_iter;
        double ch_x = h.get<changing_value>().v * glm::backEaseOut(alpha)
                      + h.get<start_value>().v;
        cur_iter += 1.;
        notepad::push_command(
            [ch_uuid = h.registry()->ctx().get<static_control_handler::id_t>(
                 entt::hashed_string{"character_uuid"}),
             k_uuid = h.registry()->ctx().get<static_control_handler::id_t>(
                 entt::hashed_string{"kitten_uuid"}),
             ch_x](notepad* np, scintilla* /*sct*/) {
                auto find = [np](auto uuid) {
                    return std::ranges::find_if(
                        np->static_controls,
                        [uuid](auto& w) { return w.get_id() == uuid; });
                };
                auto k = find(k_uuid);
                auto ch = find(ch_uuid);
                if(ch == np->static_controls.end()
                   || k == np->static_controls.end()) {
                    return;
                }
                ch->position.x = static_cast<int32_t>(glm::round(ch_x));
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

void good_end_animation(entt::handle end_anim) {
    using namespace std::chrono_literals;
    end_anim.emplace<timeline::eval_direction>();
    end_anim.emplace<total_iterations>(static_cast<double>(4s / timings::dt)
                                       * 1.);
    end_anim.emplace<current_iteration>(0.);
    static constexpr double start_x = 20.;
    static constexpr double x_duration = 280.;
    end_anim.emplace<changing_value>(x_duration);
    end_anim.emplace<start_value>(start_x);
    end_anim.emplace<timeline::what_do>(+[](const void*, entt::handle h,
                                            timeline::direction,
                                            timings::duration) {
        auto& cur_iter = h.get<current_iteration>().v;
        auto total_iter = h.get<total_iterations>().v;
        double alpha = std::min(total_iter, cur_iter) / total_iter;
        static constexpr double kitten_diff = 30.;
        auto start_x = h.get<start_value>().v;
        auto changing_x = h.get<changing_value>().v;
        double ch_x = changing_x * glm::quarticEaseOut(alpha) + start_x;
        double k_x = -1 * (changing_x - kitten_diff) * glm::cubicEaseOut(alpha)
                     + h.get<start_value>().v + changing_x + changing_x;

        cur_iter += 1.;
        notepad::push_command(
            [ch_uuid = h.registry()->ctx().get<static_control_handler::id_t>(
                 entt::hashed_string{"character_uuid"}),
             k_uuid = h.registry()->ctx().get<static_control_handler::id_t>(
                 entt::hashed_string{"kitten_uuid"}),
             k_x, ch_x](notepad* np, scintilla* /*sct*/) {
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
                ch->position.x = static_cast<int32_t>(glm::round(ch_x));
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

void create_input_wait(entt::registry& reg, game_status_flag status) {
    reg.ctx().emplace_as<game_status_flag>(
        entt::hashed_string{"flag_for_set"}) = status;
    reg.clear<input::key_down_task>();
    auto inpt = reg.create();
    reg.emplace<input::key_down_task>(
        inpt, +[](const void*, entt::handle h, std::span<input::key_state>) {
            notepad::push_command([](notepad* np, scintilla* /*sct*/) {
                np->static_controls.clear();
            });
            h.registry()->ctx().get<game_status_flag>() =
                h.registry()->ctx().get<game_status_flag>(
                    entt::hashed_string{"flag_for_set"});
        });
    notepad::push_command([](notepad* np, scintilla* sct) {
        static constexpr pos msg_pos{450, 0};
        static constexpr pos w_size = pos(500, 50);
        auto ctrl = static_control_handler{}
                        .with_position(msg_pos)
                        .with_size(w_size)
                        .with_text("Press any key to Restart")
                        .with_text_color(sct->get_text_color(STYLE_DEFAULT));
        np->show_static_control(std::move(ctrl));
        // SetWindowText(ctrl, ctrl.text.data());
    });
}

} // namespace game_over
;
namespace kitten {
void on_collide(const void* /*payload*/, entt::registry& reg,
                collision::self self, collision::collider collider) {
    using namespace std::chrono_literals;
    static constexpr auto END_ANIM_DURATION = 5s;
    if(reg.all_of<projectile::projectile_tag>(collider)) {
        factories::emplace_simple_death_anim({reg, self});
        reg.emplace<life::begin_die>(self);
        reg.emplace_or_replace<life::begin_die>(collider);
        reg.emplace<life::begin_die>(
            reg.ctx().get<entt::entity>(entt::hashed_string{"character_id"}));
        reg.ctx().erase<entt::entity>(entt::hashed_string{"character_id"});
        auto char_wnd = game_over::make_character();
        reg.ctx().emplace_as<static_control_handler::id_t>(
            entt::hashed_string{"character_uuid"}) = char_wnd.get_id();

        static constexpr pos dead_kitten_start_pos{300, 0};
        static constexpr pos w_size = pos(150, 100);
        auto kitten =
            static_control_handler{}
                .with_position(dead_kitten_start_pos)
                .with_text("___")
                //.with_text_color(ALL_COLORS['_' - PRINTABLE_RANGE.first - 1])
                .with_size(w_size);
        reg.ctx().emplace_as<static_control_handler::id_t>(
            entt::hashed_string{"kitten_uuid"}) = kitten.get_id();
        game_over::push_controls(
            std::to_array({std::move(char_wnd), std::move(kitten)}));
        const auto end_anim = entt::handle{reg, reg.create()};
        game_over::bad_end_animation(end_anim);

        timer::make(
            end_anim,
            +[](const void*, entt::handle h) {
                game_over::create_input_wait(*h.registry(),
                                             game_over::game_status_flag::find);
            },
            END_ANIM_DURATION);
    }
    if(reg.all_of<character::character_tag>(collider)) {
        reg.emplace<life::begin_die>(self);
        reg.emplace<life::begin_die>(collider);
        auto char_wnd = game_over::make_character();
        reg.ctx().emplace_as<static_control_handler::id_t>(
            entt::hashed_string{"character_uuid"}) = char_wnd.get_id();
        const auto end_anim = entt::handle{reg, reg.create()};
        const auto sprt = end_anim.emplace<sprite>(reg.get<sprite>(self));
        static constexpr pos kitten_start_pos(300 + 280, 0);
        static constexpr pos kitten_size(50, 50);
        auto kitten_wnd =
            static_control_handler{}
                .with_position(kitten_start_pos)
                //.with_text_color(
                //    ALL_COLORS[sprt.data()[0] - PRINTABLE_RANGE.first - 1])
                .with_text(sprt.data())
                .with_size(kitten_size);

        reg.ctx().emplace_as<static_control_handler::id_t>(
            entt::hashed_string{"kitten_uuid"}) = kitten_wnd.get_id();
        game_over::push_controls(
            std::to_array({std::move(char_wnd), std::move(kitten_wnd)}));

        game_over::good_end_animation(end_anim);
        using namespace std::chrono_literals;
        timer::make(
            end_anim,
            +[](const void*, entt::handle h) {
                game_over::create_input_wait(*h.registry(),
                                             game_over::game_status_flag::kill);
            },
            END_ANIM_DURATION);
    }
};
void make(entt::handle h, loc loc, sprite sp) {
    h.emplace<kitten_tag>();
    h.emplace<collision::hit_extends>(sp.bounds());
    factories::make_base_renderable(h, loc, 1, std::move(sp));
    h.emplace<collision::agent>();
    h.emplace<collision::responce_func>(
        collision::responce_func::function_type{kitten::on_collide});
}
} // namespace kitten

namespace timeline {
void make(entt::handle h, timeline::what_do::function_type what_do,
          std::chrono::milliseconds duration, direction dir) {
    h.emplace<life::life_time>(duration);
    h.emplace<timeline::what_do>(what_do);
    h.emplace<timeline::eval_direction>(dir);
}

} // namespace timeline

namespace atmosphere {

[[maybe_unused]] const auto time_between_cycle = std::chrono::seconds{20};
const auto cycle_duration = std::chrono::seconds{2};

void update_cycle(const void* /*payload*/, entt::handle h,
                  timeline::direction d, timings::duration /*unused*/) {
    const auto& [start, end] = h.get<color_range>();
    const auto& [current_life_time] = h.get<life::life_time>();

    double value = (current_life_time - cycle_duration)
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

    notepad::push_command([new_back_color, new_front_color](notepad* np,
                                                            scintilla* sct) {
        // np->back_color = new_back_color;
        for(auto& w: np->static_controls) {
            w.fore_color = new_front_color;
            SetWindowText(w.get_wnd(), w.text.data());
        }
        for(auto i: {0, STYLE_DEFAULT}) {
            sct->force_set_back_color(i, new_back_color);
            sct->force_set_text_color(i, new_front_color);
        }
        // FIXME(Igor): All styles constant without computation
        int style = MY_STYLE_START + PRINTABLE_RANGE.first;
        for(size_t i = 0; i < PRINTABLE_RANGE.second - PRINTABLE_RANGE.first;
            i++, style++) {
            sct->force_set_back_color(style, new_back_color);
        }
    });
}

void run_cycle(entt::handle timer, color_range range) {
    const auto cycle_timeline =
        entt::handle{*timer.registry(), timer.registry()->create()};

    timeline::make(cycle_timeline, &atmosphere::update_cycle, cycle_duration,
                   timer.get<timeline::eval_direction>().v);

    cycle_timeline.emplace<color_range>(range);
    cycle_timeline.emplace<life::dying_wish>(
        life::dying_wish::function_type{[](const void*, entt::handle h) {
            const auto again_timer =
                entt::handle{*h.registry(), h.registry()->create()};
            again_timer.emplace<color_range>(h.get<color_range>());
            timer::make(
                again_timer,
                +[](const void*, entt::handle timer) {
                    auto range = timer.get<color_range>();
                    run_cycle(timer, range);
                },
                time_between_cycle);
            again_timer.emplace<timeline::eval_direction>(
                timeline::invert(h.get<timeline::eval_direction>().v));
        }});
}

void make(entt::handle h) {
    std::mutex m;
    std::condition_variable cv;
    COLORREF back_color = 0;
    COLORREF fore_color = 0;
    bool ready = false;
    notepad::push_command([&back_color, &fore_color, &ready, &m,
                           &cv](notepad* /*np*/, scintilla* sct) {
        back_color = sct->get_background_color(STYLE_DEFAULT);
        fore_color = sct->get_text_color(STYLE_DEFAULT);
        {
            std::lock_guard lk(m);
            ready = true;
        }
        cv.notify_one();
    });
    std::unique_lock lk(m);
    cv.wait(lk, [&ready] { return ready; });
    h.registry()->ctx().emplace_as<COLORREF>(
        entt::hashed_string{"start_back_color"}) = back_color;
    h.registry()->ctx().emplace_as<COLORREF>(
        entt::hashed_string{"start_fore_color"}) = fore_color;
    timer::make(
        h,
        +[](const void*, entt::handle timer) {
            run_cycle(timer, color_range{
                                 timer.registry()->ctx().get<COLORREF>(
                                     entt::hashed_string{"start_fore_color"}),
                                 timer.registry()->ctx().get<COLORREF>(
                                     entt::hashed_string{"start_back_color"})});
            timer.registry()->ctx().erase<COLORREF>(
                entt::hashed_string{"start_back_color"});
            timer.registry()->ctx().erase<COLORREF>(
                entt::hashed_string{"start_fore_color"});
        },
        time_between_cycle);
    h.emplace<timeline::eval_direction>(timeline::direction::reverse);
}
} // namespace atmosphere
