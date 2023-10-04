#pragma once
#include <array>
#include <chrono>
#include <string>
#include <string_view>
#include <random>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>

#include <Windows.h>

#include "engine/buffer.h"
#include "engine/details/base_types.hpp"
#include "engine/time.h"
#include "game/factories.h"
#include "game/ecs_processors/collision.h"
#include "game/ecs_processors/drawers.h"
#include "game/ecs_processors/input.h"
#include "game/ecs_processors/motion.h"
#include "game/ecs_processors/life.h"
#include "engine/scintilla_wrapper.h"
#include "engine/notepad.h"
#include "engine/world.h"
#include "game/lexer.h"
#include "messages.h"

namespace game {

static std::array messages = std::to_array<std::string_view>(ALL_GAME_MESSAGES);
using rng_type = std::mt19937;
static std::uniform_int_distribution<rng_type::result_type>
    dist(0, static_cast<int>(messages.size() - 1));
static rng_type rng{};

static lexer game_lexer{};

inline void define_all_styles(scintilla* sc) {
    int style = my_style_start + printable_range.first;
    sc->dcall0(SCI_STYLECLEARALL);
    for(auto i: ALL_COLORS) {
        sc->dcall2(SCI_STYLESETFORE, style, i);
        style++;
    }
}
inline void run(pos game_area, back_buffer& game_buffer);
inline void start(pos game_area,
                  // NOLINTNEXTLINE(performance-unnecessary-value-param)
                  std::shared_ptr<std::atomic_bool> shutdown) {
    notepad::push_command([](notepad*, scintilla* sc) {
        sc->set_lexer(&game_lexer);
        define_all_styles(sc);
    });
    back_buffer game_buffer{game_area};
    while(!shutdown->load()) {
        run(game_area, game_buffer);
        game_buffer.clear();
        // restart
    }
};
inline void run(pos game_area, back_buffer& game_buffer) {
    world w{};
    auto& exec = w.procs;
    exec.emplace_back(input::processor{});
    exec.emplace_back(uniform_motion{});
    exec.emplace_back(non_uniform_motion{});
    exec.emplace_back(timeline::executor{});
    exec.emplace_back(rotate_animator{});
    exec.emplace_back(collision::query(w, game_area));
    exec.emplace_back(redrawer(game_buffer, w));
    exec.emplace_back(life::death_last_will_executor{});
    exec.emplace_back(life::killer{});
    exec.emplace_back(life::life_ticker{});

    w.spawn_actor([](entt::registry& reg, const entt::entity e) {
        atmosphere::make(reg, e);
    });

    constexpr int ITEMS_COUNT = 200;
    constexpr int character_i = 199;
    constexpr int kitten_i = 198;
    constexpr int gun_i = 197;
    std::array<pos, ITEMS_COUNT> all{};
    std::mt19937 gen(std::random_device{}());
    {
        std::uniform_int_distribution<> dist_x(0, game_area.x - 1 - 1);
        std::uniform_int_distribution<> dist_y(0, game_area.y - 1);
        for(auto i = 0; i < ITEMS_COUNT; i++) {
            pos p;
            do {
                p = pos{dist_x(gen), dist_y(gen)};
            } while(
                std::ranges::any_of(all.begin(), all.begin() + i,
                                    [p](auto other) { return p == other; }));
            all[i] = p;
        }
    }
    auto msg_w = static_control{};
    auto w_uuid = msg_w.get_id();
    notepad::push_command(
        [msg_w = std::move(msg_w)](notepad* np, scintilla* sc) mutable {
            static constexpr int height = 100;
            static constexpr int wnd_x = 20;
            msg_w.with_size(pos(sc->get_window_width(), height))
                .with_position(pos(wnd_x, 0))
                .with_text_color(RGB(0, 0, 0))
                .show(np);
            np->static_controls.emplace_back(std::move(msg_w));
        });
    std::uniform_int_distribution<> dist_ch(printable_range.first,
                                            printable_range.second);
    for(size_t i = 0; i < all.size() - 3; i++) {
        auto item = all[i];
        w.spawn_actor([&](entt::registry& reg, const entt::entity ent) {
            std::string s{' '};
            do {
                s[0] = static_cast<char>(dist_ch(gen));
            } while(s[0] == '#' || s[0] == '-' || s[0] == '_');
            actor::make_base_renderable(
                reg, ent, item, 3,
                sprite(sprite::unchecked_construct_tag{}, s));
            reg.emplace<collision::agent>(ent);
            reg.emplace<collision::on_collide>(
                ent, [w_uuid](entt::registry& reg, collision::self self,
                              collision::collider collider) {
                    if(reg.all_of<character>(collider)) {
                        char ascii_mesh = reg.get<sprite>(self).data()[0];
                        const auto l = reg.get<const loc>(self);
                        rng.seed(static_cast<int64_t>(
                            static_cast<int>(ascii_mesh) + l.y + l.x));
                        size_t rand_msg = dist(rng);
                        notepad::push_command(
                            [w_uuid, msg = messages[rand_msg]](notepad* np,
                                                               scintilla*) {
                                auto w = std::ranges::find_if(
                                    np->static_controls, [w_uuid](auto& w) {
                                        return w.get_id() == w_uuid;
                                    });
                                SetWindowText(*w, msg.data());
                                w->text = msg;
                            });
                    }
                    if(reg.all_of<projectile>(collider)) {
                        reg.emplace_or_replace<life::begin_die>(self);
                    }
                    return collision::responce::block;
                });
            emplace_simple_death(reg, ent);
        });
    }

    game_status_flag game_flag = game_status_flag::unset;
    w.spawn_actor([kitten_pos = all[kitten_i], kitten_mesh = dist_ch(gen),
                   &game_flag](entt::registry& reg, const entt::entity e) {
#ifndef NDEBUG
        static constexpr auto debug_kitten_pos = pos{10, 30};
        kitten::make(reg, e, debug_kitten_pos,
                     sprite(sprite::unchecked_construct_tag{}, "Q"), game_flag);
#else
        kitten::make(reg, e, kitten_pos, kitten_mesh);
#endif
    });
    w.spawn_actor([gun_pos = all[gun_i], gun_mesh = dist_ch(gen)](
                      entt::registry& reg, const entt::entity e) {
#ifndef NDEBUG
        static constexpr auto debug_gun_pos = pos{14, 20};
        gun::make(reg, e, debug_gun_pos,
                  sprite(sprite::unchecked_construct_tag{}, "<"));
#else
        gun::make(reg, e, gun_pos, gun_mesh);
#endif
    });
    w.spawn_actor([char_pos = all[character_i]
                   ](entt::registry& reg, const entt::entity ent) {
        reg.emplace<sprite>(ent,
                            sprite(sprite::unchecked_construct_tag{}, "#"));

#ifndef NDEBUG
        static constexpr auto debug_character_pos = pos{10, 25};
        character::make(reg, ent, debug_character_pos);
#else
        character::make(reg, ent, char_pos);
        notepad::push_command([char_pos, game_area](notepad*, scintilla* sc) {
            auto height = sc->get_lines_on_screen();
            auto width = sc->get_window_width() / sc->get_char_width();
            sc->scroll(
                std::max(0, static_cast<int>(std::min(char_pos.x - width / 2,
                                                      game_area.x - width))),
                std::max(0, std::min(char_pos.y - height / 2,
                                     game_area.y - height)));
        });
#endif
        auto& [input_callback] =
            reg.emplace<input::processor::down_signal>(ent);
        input_callback.connect(&character::process_movement_input<>);
    });

    timings::fixed_time_step fixed_time_step{};
    timings::fps_count fps_count{};

    while(game_flag == game_status_flag::unset) {
        fixed_time_step.sleep();
        fps_count.fps([](auto fps) {
            notepad::push_command([fps](notepad* np, scintilla*) {
                np->window_title.game_thread_fps =
                    static_cast<decltype(np->window_title.game_thread_fps)>(
                        fps);
            });
        });
        // TODO(Igor): real alpha
        w.tick(timings::dt);
        // render
        notepad::push_command([&game_buffer](notepad* /*np*/, scintilla* sc) {
            // swap buffers in Scintilla
            const auto pos = sc->get_caret_index();
            game_buffer.view([sc](const std::basic_string<char_size>& buf) {
                sc->set_new_all_text(buf);
            });
            sc->set_caret_index(pos);
        });
    }
};
} // namespace game
