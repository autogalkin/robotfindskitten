#include <array>
#include <condition_variable>
#include <utility>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>

#include "Windows.h"
#include <winuser.h>

#include "engine/notepad.h"
#include "game/factories.h"
#include "game/lexer.h"

namespace factories {
void emplace_simple_death_anim(entt::registry& reg, const entt::entity e) {
    reg.emplace<life::dying_wish>(e, [](entt::registry& reg_,
                                             const entt::entity self) {
        const auto dead = reg_.create();
        auto old_loc = reg_.get<loc>(self);
        factories::make_base_renderable(
            reg_, dead, old_loc - loc(1, 0), 1,
            sprite(sprite::unchecked_construct_tag{}, "___"));
        reg_.emplace<life::life_time>(dead, std::chrono::seconds{3});
        reg_.emplace<timeline::eval_direction>(dead);
        reg_.emplace<timeline::what_do>(
            dead,
            [](entt::registry& r_, const entt::entity e_,
               timeline::direction /**/) { r_.get<translation>(e_).mark(); });
    });
}

} // namespace factories
namespace projectile {
namespace {
using rng_type = std::mt19937;
rng_type rng{}; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
} // namespace
collision::responce on_collide(entt::registry& r, collision::self self,
                               collision::collider other) {
    if(r.all_of<projectile_tag>(other)) {
        return collision::responce::ignore;
    }
    r.emplace_or_replace<life::begin_die>(self);
    return collision::responce::block;
}

void make(entt::registry& reg, entt::entity e, loc start, velocity dir,
          std::chrono::milliseconds life_time) {
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

    reg.emplace<life::life_time>(e, life_time);

    // NOLINTBEGIN(readability-magic-numbers)
    static std::uniform_int_distribution<projectile::rng_type::result_type>
        fall_dist(3, 7);
    reg.emplace<timeline::what_do>(
        e,
        [total_iterations = static_cast<double>((life_time / timings::dt)) * 1.,
         current_iteration = 0., start_y = start.y,
         changing_y = fall_dist(projectile::rng),
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
                    e_, std::exchange(
                            reg_.get<sprite>(e_),
                            sprite(sprite::unchecked_construct_tag{}, "_")));
                call = false;
                // NOLINTEND( bugprone-narrowing-conversions)
            }
            current_iteration += 1.;
        // NOLINTEND(readability-magic-numbers)
        });

    reg.emplace<timeline::eval_direction>(e, timeline::direction::forward);
    reg.emplace<life::dying_wish>(e, [](entt::registry& reg_,
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
        reg_.emplace<life::life_time>(new_e, std::chrono::seconds{1});

        reg_.emplace<life::dying_wish>(new_e, [](entt::registry& r_,
                                                      const entt::entity ent_) {
            auto& lb = r_.get<loc>(ent_);
            auto& trans = r_.get<translation>(ent_);

            auto create_fragment = [&r_, lb, trans](loc offset, velocity dir_) {
                const entt::entity proj = r_.create();
                r_.emplace<projectile_tag>(proj);
                r_.emplace<sprite>(proj, sprite::unchecked_construct_tag{},
                                   ".");
                r_.emplace<loc>(proj, lb + offset);
                r_.emplace<translation>(proj, trans.get());
                r_.emplace<visible_in_game>(proj);
                r_.emplace<z_depth>(proj, 0);
                r_.emplace<velocity>(proj, dir_);
                r_.emplace<life::life_time>(proj, std::chrono::seconds{1});
                r_.emplace<non_uniform_movement_tag>(proj);
                r_.emplace<collision::agent>(proj);
                r_.emplace<collision::on_collide>(proj,
                                                  &projectile::on_collide);
            };

            create_fragment({0., 1.}, {0, 2});
            create_fragment({0., -1.},
                            {0., -2.}); // NOLINT(readability-magic-numbers)
            create_fragment({0, 0}, {1, 0});
            create_fragment({-1, 0}, {-1, 0});
        });
    });
}
} // namespace projectile

namespace gun {
collision::responce on_collide(entt::registry& reg, const collision::self self,
                               const collision::collider collider) {
    if(reg.all_of<character::character_tag>(collider)) {
        reg.emplace_or_replace<life::begin_die>(self);
        return collision::responce::ignore;
    }
    return collision::responce::block;
}

void make(entt::registry& reg, const entt::entity e, loc loc, sprite sp) {
    reg.emplace<gun_tag>(e);
    factories::make_base_renderable(reg, e, loc, 1, std::move(sp));
    reg.emplace<collision::agent>(e);
    reg.emplace<collision::on_collide>(e, &gun::on_collide);
}

void fire(entt::registry& reg, const entt::entity character) {
    const auto& l = reg.get<loc>(character);
    auto& sh = reg.get<sprite>(character);
    auto dir = reg.get<draw_direction>(character);

    const auto proj = reg.create();
    const loc spawn_translation =
        dir == draw_direction::right ? loc(sh.bounds().x, 0) : loc(-1, 0);
    static constexpr pos random_end_y{3, 5};
    static constexpr pos random_velocity_range{15, 25};
    static std::uniform_int_distribution<projectile::rng_type::result_type>
        life_dist(random_end_y.x, random_end_y.y);
    static std::uniform_int_distribution<projectile::rng_type::result_type>
        velocity_dist(random_velocity_range.x,random_velocity_range.y);
    projectile::make(
        reg, proj, l + spawn_translation,
        velocity(velocity_dist(projectile::rng) * static_cast<double>(dir), 0),
        std::chrono::seconds{life_dist(projectile::rng)});
}
} // namespace gun

namespace character {

collision::responce on_collide(entt::registry& reg, const collision::self self,
                               const collision::collider collider) {
    // TODO(Igor) collision functions without cast?
    if(reg.all_of<gun::gun_tag>(collider)) {
        auto& animation = reg.get<sprite>(self);
        auto dir = reg.get<draw_direction>(self);
        reg.emplace<previous_sprite>(
            self,
            std::exchange(animation,
                          sprite(sprite::unchecked_construct_tag{},
                                 dir == draw_direction::left ? "-#" : "#-")));
        reg.emplace<on_change_direction>(
            self, [self, &reg](draw_direction new_dr, sprite& sp) {
                const auto* s = new_dr == draw_direction::left ? "-#" : "#-";
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
                //i // {r.right-r.left, r.bottom-r.top}
                // FIXME(Igor): implicit
                if(i.fore_color == RGB(0, 0, 0)) {
                    i.fore_color = sct->get_text_color(STYLE_DEFAULT);
                }
                np->show_static_control(std::move(i));
            }
        });
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
void bad_end_animation(static_control_handler::id_t char_wnd_id,
                       static_control_handler::id_t kitten_wnd_id, entt::registry& reg,
                       entt::entity end_anim) {
    reg.emplace<timeline::eval_direction>(end_anim);
    using namespace std::chrono_literals;
    static constexpr int start_x = 20;
    static constexpr int x_duration = 200;
    reg.emplace<timeline::what_do>(
        end_anim,
        [ch_uuid = char_wnd_id, k_uuid = kitten_wnd_id,
         total_iterations = static_cast<double>(4s / timings::dt) * 1.,
         current_iteration = 0., start_x = start_x, changing_x = x_duration](
            entt::registry& /*r*/, const entt::entity /*e*/,
            timeline::direction /**/) mutable {
            double alpha = std::min(total_iterations, current_iteration)
                           / total_iterations;
            double ch_x = changing_x * glm::backEaseOut(alpha) + start_x;
            current_iteration += 1.;
            notepad::push_command(
                [ch_uuid, k_uuid, ch_x](notepad* np, scintilla* /*sct*/) {
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

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
void good_end_animation(static_control_handler::id_t char_wnd_id,
                        static_control_handler::id_t kitten_wnd_id, entt::registry& reg,
                        entt::entity end_anim) {
    using namespace std::chrono_literals;
    reg.emplace<timeline::eval_direction>(end_anim);

    static constexpr int start_x = 20;
    static constexpr int x_duration = 280;
    reg.emplace<timeline::what_do>(
        end_anim,
        [ch_uuid = char_wnd_id, k_uuid = kitten_wnd_id,
         total_iterations = static_cast<double>(4s / timings::dt) * 1.,
         current_iteration = 0., start_x = start_x, changing_x = x_duration](
            entt::registry& /*r*/, const entt::entity /*e*/,
            timeline::direction /**/) mutable {
            double alpha = std::min(total_iterations, current_iteration)
                           / total_iterations;
            double ch_x = changing_x * glm::quarticEaseOut(alpha) + start_x;
            static constexpr int kitten_diff = 30;
            double k_x =
                -1 * (changing_x - kitten_diff) * glm::cubicEaseOut(alpha)
                + start_x + x_duration + x_duration;

            current_iteration += 1.;
            notepad::push_command(
                [ch_uuid, k_uuid, k_x, ch_x](notepad* np, scintilla* /*sct*/) {
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

void create_input_wait(entt::registry& reg, game_status_flag status,
                       game_status_flag& game_over_flag) {
    auto ent = reg.create();
    auto& [input_callback] = reg.emplace<input::processor::down_signal>(ent);
    input_callback.connect(
        [&game_over_flag, status](
            entt::registry& /*reg*/, const entt::entity /*e*/,
            const input::state_t& /*state*/, const timings::duration /*dt*/) {
            notepad::push_command([](notepad* np, scintilla* /*sct*/) {
                np->static_controls.clear();
            });
            game_over_flag = status;
        });
    notepad::push_command([](notepad* np, scintilla* sct) {
        static constexpr pos msg_pos{450, 0};
        static constexpr pos w_size = pos(500, 50);
        auto ctrl = static_control_handler{}
                        .with_position(msg_pos)
                        .with_size(w_size)
                        .with_text("Press any key to Restart")
                        .with_text_color(sct->get_text_color(STYLE_DEFAULT));
        SetWindowText(ctrl, ctrl.text.data());
        np->show_static_control(std::move(ctrl));
    });
}

} // namespace game_over

namespace kitten {
collision::responce on_collide(entt::registry& reg, collision::self self,
                               collision::collider collider,
                               game_over::game_status_flag& game_over_flag, entt::entity character_entity) {
    using namespace std::chrono_literals;
    static constexpr auto end_anim_duration = 5s;
    if(reg.all_of<projectile::projectile_tag>(collider)) {
        factories::emplace_simple_death_anim(reg, self);
        reg.emplace<life::begin_die>(self);
        reg.emplace<life::begin_die>(collider);
        reg.emplace<life::begin_die>(character_entity);
        auto char_wnd = game_over::make_character();
        auto ch_uuid = char_wnd.get_id();
        static constexpr pos dead_kitten_start_pos{300, 0};
        static constexpr pos w_size = pos(150, 100);
        auto kitten =
            static_control_handler{}
                .with_position(dead_kitten_start_pos)
                .with_text("___")
                //.with_text_color(ALL_COLORS['_' - PRINTABLE_RANGE.first - 1])
                .with_size(w_size);
        auto k_uuid = kitten.get_id();
        game_over::push_controls(
            std::to_array({std::move(char_wnd), std::move(kitten)}));
        const auto end_anim = reg.create();
        game_over::bad_end_animation(ch_uuid, k_uuid, reg, end_anim);

        timer::make(
            reg, end_anim,
            [&game_over_flag](entt::registry& reg, entt::entity /*timer*/) {
                game_over::create_input_wait(
                    reg, game_over::game_status_flag::find, game_over_flag);
            },
            end_anim_duration);
    }
    if(reg.all_of<character::character_tag>(collider)) {
        reg.emplace<life::begin_die>(self);
        reg.emplace<life::begin_die>(collider);
        auto char_wnd = game_over::make_character();
        auto ch_uuid = char_wnd.get_id();
        const auto end_anim = reg.create();
        const auto sprt = reg.emplace<sprite>(end_anim, reg.get<sprite>(self));
        static constexpr pos kitten_start_pos(300 + 280, 0);
        static constexpr pos kitten_size(50, 50);
        auto kitten_wnd =
            static_control_handler{}
                .with_position(kitten_start_pos)
                //.with_text_color(
                //    ALL_COLORS[sprt.data()[0] - PRINTABLE_RANGE.first - 1])
                .with_text(sprt.data())
                .with_size(kitten_size);

        auto k_uuid = kitten_wnd.get_id();
        game_over::push_controls(
            std::to_array({std::move(char_wnd), std::move(kitten_wnd)}));

        game_over::good_end_animation(ch_uuid, k_uuid, reg, end_anim);
        using namespace std::chrono_literals;
        timer::make(
            reg, end_anim,
            [&game_over_flag](entt::registry& reg, entt::entity /*timer*/) {
                game_over::create_input_wait(
                    reg, game_over::game_status_flag::kill, game_over_flag);
            },
            end_anim_duration);
    }
    return collision::responce::ignore;
};
void make(entt::registry& reg, entt::entity e, loc loc, sprite sp,
          game_over::game_status_flag& game_over_flag, entt::entity character_entity) {
    reg.emplace<kitten_tag>(e);
    factories::make_base_renderable(reg, e, loc, 1, std::move(sp));
    reg.emplace<collision::agent>(e);
    reg.emplace<collision::on_collide>(
        e, collision::on_collide{[&game_over_flag, character_entity](
                                     entt::registry& reg, collision::self self,
                                     collision::collider collider) {
            return kitten::on_collide(reg, self, collider, game_over_flag, character_entity);
        }});
}
} // namespace kitten

namespace timeline {
void make(
    entt::registry& reg, entt::entity e,
    std::function<void(entt::registry&, entt::entity, direction)>&& what_do,
    const std::chrono::milliseconds duration, direction dir) {
    reg.emplace<life::life_time>(e, duration);
    reg.emplace<timeline::what_do>(e, std::move(what_do));
    reg.emplace<timeline::eval_direction>(e, dir);
}

} // namespace timeline

namespace atmosphere {

const auto time_between_cycle = std::chrono::seconds{20};
const auto cycle_duration = std::chrono::seconds{2};

void update_cycle(entt::registry& reg, entt::entity e, timeline::direction d) {
    const auto& [start, end] = reg.get<color_range>(e);
    const auto& [current_life_time] = reg.get<life::life_time>(e);

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

    notepad::push_command(
        [new_back_color, new_front_color](notepad* np, scintilla* sct) {
            // np->back_color = new_back_color;
            for(auto& w: np->static_controls) {
                w.fore_color = new_front_color;
                SetWindowText(w.get_wnd(), w.text.data());
            }
            for(auto i: {0, STYLE_DEFAULT}) {
                sct->force_set_back_color(i, new_back_color);
                sct->force_set_text_color(i, new_front_color);
            }
            // FIXME(Igor): All styles constant
            int style = MY_STYLE_START + PRINTABLE_RANGE.first;
            for(size_t i = 0; i < PRINTABLE_RANGE.second - PRINTABLE_RANGE.first; i++, style++) {
                sct->force_set_back_color(style, new_back_color);
            }
        });
}

void run_cycle(entt::registry& reg, entt::entity timer, color_range range) {
    const auto cycle_timeline = reg.create();

    timeline::make(reg, cycle_timeline, &atmosphere::update_cycle,
                   cycle_duration,
                   reg.get<timeline::eval_direction>(timer).value);

    reg.emplace<color_range>(cycle_timeline, range);
    reg.emplace<life::dying_wish>(
        cycle_timeline, [](entt::registry& reg_, entt::entity cycle_timeline_) {
            const auto again_timer = reg_.create();
            timer::make(
                reg_, again_timer,
                [](entt::registry& reg, entt::entity timer) {
                    run_cycle(reg, timer, color_range{});
                },
                time_between_cycle);
            reg_.emplace<timeline::eval_direction>(
                again_timer,
                timeline::invert(
                    reg_.get<timeline::eval_direction>(cycle_timeline_).value));
        });
}

void make(entt::registry& reg, entt::entity e) {
    std::mutex m;
    std::condition_variable cv;
    COLORREF back_color = RGB(255, 0, 0);
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

    timer::make(
        reg, e,
        [back_color](entt::registry& reg, entt::entity timer) {
            run_cycle(reg, timer, color_range{RGB(0, 0, 0), back_color});
        },
        time_between_cycle);
    reg.emplace<timeline::eval_direction>(e, timeline::direction::reverse);
}
} // namespace atmosphere
