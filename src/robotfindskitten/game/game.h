#pragma once
#ifndef _CPP_PROJECTS_ROBOTFINDSKITTEN_SRC_ROBOTFINDSKITTEN_GAME_GAME_H
#define _CPP_PROJECTS_ROBOTFINDSKITTEN_SRC_ROBOTFINDSKITTEN_GAME_GAME_H

#include <array>
#include <chrono>
#include <random>
#include <span>
#include <string>
#include <string_view>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <glm/gtx/hash.hpp>

#include <Windows.h>
#include <entt/entity/fwd.hpp>

#include "engine/buffer.h"
#include "engine/details/base_types.hpp"
#include "engine/notepad.h"
#include "engine/scintilla_wrapper.h"
#include "engine/time.h"
#include "engine/world.h"
#include "game/ecs_processors/collision_query.h"
#include "game/ecs_processors/drawers.h"
#include "game/ecs_processors/input.h"
#include "game/ecs_processors/life.h"
#include "game/ecs_processors/motion.h"
#include "game/factories.h"
#include "game/lexer.h"
#include "messages.h"

namespace game {

static inline constexpr int COLOR_COUNT = 255;

using color_t = decltype(RGB(0, 0, 0));
using all_colors_t =
    std::array<color_t, PRINTABLE_RANGE.second - PRINTABLE_RANGE.first - 1>;

inline all_colors_t generate_colors() {
    std::random_device rd{};
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distr(0, COLOR_COUNT);
    auto arr = std::array<decltype(RGB(0, 0, 0)),
                          PRINTABLE_RANGE.second - PRINTABLE_RANGE.first - 1>();
    for(auto& i: arr) {
        i = RGB(distr(gen), distr(gen), distr(gen));
    }
    return arr;
}

static const std::array MESSAGES =
    std::to_array<std::string_view>(ALL_GAME_MESSAGES);

inline void define_all_styles(scintilla* sc,
                              std::span<const color_t> all_colors) {
    int style = MY_STYLE_START + PRINTABLE_RANGE.first;
    sc->clear_all_styles();
    for(auto i: all_colors) {
        sc->set_text_color(style, i);
        style++;
    }
}
using buffer_type = back_buffer<thread_safe_trait<std::mutex>>;
inline void run(pos game_area, buffer_type& game_buffer);
inline void start(pos game_area, std::shared_ptr<std::atomic_bool>&& shutdown) {
    static lexer GAME_LEXER{};
    notepad::push_command(
        [](notepad*, scintilla* sc) { sc->set_lexer(&GAME_LEXER); });
    auto game_buffer = back_buffer<thread_safe_trait<std::mutex>>{game_area};
    while(!shutdown->load()) {
        run(game_area, game_buffer);
        auto lock = game_buffer.lock();
        game_buffer.clear();
        // restart
    }
};
inline void run(pos game_area, buffer_type& game_buffer) {
    const all_colors_t ALL_COLORS = generate_colors();
    notepad::push_command([&ALL_COLORS](notepad*, scintilla* sc) {
        define_all_styles(sc, ALL_COLORS);
    });
    world w{};
    auto& exec = w.procs;
    exec.emplace_back(input::processor{});
    exec.emplace_back(uniform_motion{});
    exec.emplace_back(non_uniform_motion{});
    exec.emplace_back(timeline::executor{});
    exec.emplace_back(rotate_animator{});
    exec.emplace_back(collision::query(w, game_area));
    exec.emplace_back(redrawer<buffer_type>(game_buffer, w));
    exec.emplace_back(life::death_last_will_executor{});
    exec.emplace_back(life::killer{});
    exec.emplace_back(life::life_ticker{});

    w.spawn_actor([](entt::registry& reg, const entt::entity e) {
        atmosphere::make(reg, e);
    });

    constexpr int ITEMS_COUNT = 200;

    using rng_type = std::mt19937;

    std::unordered_set<pos> all{};
    all.reserve(ITEMS_COUNT);
    rng_type gen(std::random_device{}());
    {
        std::uniform_int_distribution<> dist_x(0, game_area.x - 1 - 1);
        std::uniform_int_distribution<> dist_y(0, game_area.y - 1);
        for(auto i = 0; i < ITEMS_COUNT; i++) {
            pos p;
            do {
                p = pos{dist_x(gen), dist_y(gen)};
            } while(all.contains(p));
            all.insert(p);
        }
    }
    static_control_handler::id_t w_uuid;
    {
        auto msg_w = static_control_handler{};
        w_uuid = msg_w.get_id();
        notepad::push_command(
            [msg_w = std::move(msg_w)](notepad* np, scintilla* sc) mutable {
                static constexpr int height = 100;
                static constexpr int wnd_x = 20;
                msg_w.with_size(pos(sc->get_window_width(), height))
                    .with_position(pos(wnd_x, 0))
                    .with_text_color(sc->get_text_color(STYLE_DEFAULT));
                np->show_static_control(std::move(msg_w));
            });
    }
    std::uniform_int_distribution<> dist_ch(PRINTABLE_RANGE.first + 1,
                                            PRINTABLE_RANGE.second);
    auto pos_iter = all.begin();
    for(size_t i = 0; i < all.size() - 3; i++) {
        auto item_position = *pos_iter;
        std::advance(pos_iter, 1);
        w.spawn_actor([&](entt::registry& reg, const entt::entity ent) {
            std::string s{' '};
            do {
                s[0] = static_cast<char>(dist_ch(gen));
            } while(s[0] == '#' || s[0] == '-' || s[0] == '_');
            factories::make_base_renderable(
                reg, ent, item_position, 3,
                sprite(sprite::unchecked_construct_tag{}, s));
            reg.emplace<collision::agent>(ent);
            reg.emplace<collision::on_collide>(
                ent, [w_uuid](entt::registry& reg, collision::self self,
                              collision::collider collider) {
                    if(reg.all_of<character::character_tag>(collider)) {
                        std::uniform_int_distribution<rng_type::result_type>
                            dist(0, static_cast<int>(MESSAGES.size() - 1));
                        rng_type rng{};
                        char ascii_mesh = reg.get<sprite>(self).data()[0];
                        const auto l = reg.get<const loc>(self);
                        rng.seed(static_cast<int64_t>(
                            static_cast<int>(ascii_mesh) + l.y + l.x));
                        size_t rand_msg = dist(rng);
                        notepad::push_command(
                            [w_uuid, msg = MESSAGES.at(rand_msg)](notepad* np,
                                                                  scintilla*) {
                                auto w = std::ranges::find_if(
                                    np->static_controls, [w_uuid](auto& w) {
                                        return w.get_id() == w_uuid;
                                    });
                                SetWindowText(*w, msg.data());
                                w->text = msg;
                            });
                    }
                    if(reg.all_of<projectile::projectile_tag>(collider)) {
                        reg.emplace_or_replace<life::begin_die>(self);
                    }
                    return collision::responce::block;
                });
            factories::emplace_simple_death_anim(reg, ent);
        });
    }

    w.spawn_actor([gun_pos = *pos_iter, gun_mesh = dist_ch(gen)](
                      entt::registry& reg, const entt::entity e) {
#ifndef NDEBUG
        static constexpr auto debug_gun_pos = pos{14, 20};
        gun::make(reg, e, debug_gun_pos,
                  sprite(sprite::unchecked_construct_tag{}, "<"));
#else
        std::string s{' '};
        s[0] = static_cast<char>(gun_mesh);
        gun::make(reg, e, gun_pos,
                  sprite(sprite::unchecked_construct_tag{}, s));
#endif
    });
    std::advance(pos_iter, 1);
    entt::entity char_id{};
    w.spawn_actor([&char_id, char_pos = *pos_iter,  &game_area](
                      entt::registry& reg, const entt::entity ent) {
        char_id = ent;
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

    std::advance(pos_iter, 1);
    game_over::game_status_flag game_flag = game_over::game_status_flag::unset;
    w.spawn_actor([char_id, kitten_pos = *pos_iter,
                   kitten_mesh = dist_ch(gen),
                   &game_flag](entt::registry& reg, const entt::entity e) {
#ifndef NDEBUG
        static constexpr auto debug_kitten_pos = pos{10, 30};
        kitten::make(reg, e, debug_kitten_pos,
                     sprite(sprite::unchecked_construct_tag{}, "Q"), game_flag,
                     char_id);
#else
        std::string s{' '};
        s[0] = static_cast<char>(kitten_mesh);
        kitten::make(reg, e, kitten_pos,
                     sprite(sprite::unchecked_construct_tag{}, s), game_flag,
                     char_id);
#endif
    });

    timings::fixed_time_step fixed_time_step{};
    timings::fps_count fps_count{};

    while(game_flag == game_over::game_status_flag::unset) {
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

#endif
