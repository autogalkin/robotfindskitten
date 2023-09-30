#pragma once
#include <array>

#include <Windows.h>
#include <chrono>
#include <string>
#include <string_view>
#include <random>

#include "engine/details/base_types.hpp"

#include "engine/time.h"
#include "game/ecs_processors/collision.h"
#include "game/ecs_processors/drawers.h"
#include "game/ecs_processors/input.h"
#include "game/ecs_processors/motion.h"
#include "game/ecs_processors/life.h"
#include "engine/scintilla_wrapper.h"
#include "game/factories.h"
#include "engine/notepad.h"
#include "engine/world.h"
#include "game/lexer.h"
#include "messages.h"

namespace game {

static std::array messages =
    std::to_array<std::string_view>({ALL_GAME_MESSAGES});
using rng_type = std::mt19937;
static std::uniform_int_distribution<rng_type::result_type>
    dist(0, messages.size() - 1);
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

inline void start(world& w, back_buffer& buf, notepad::commands_queue_t& cmds,
                  const pos game_area) {
    cmds.push([](notepad*, scintilla* sc) {
        sc->set_lexer(&game_lexer);
        define_all_styles(sc);
    });

    auto& exec = w.procs;
    exec.emplace_back(input::processor{});
    exec.emplace_back(uniform_motion{});
    exec.emplace_back(non_uniform_motion{});
    exec.emplace_back(timeline::executor{});
    exec.emplace_back(rotate_animator{});
    exec.emplace_back(collision::query(w, game_area));
    exec.emplace_back(redrawer(buf, w));
    exec.emplace_back(life::death_last_will_executor{});
    exec.emplace_back(life::killer{});
    exec.emplace_back(life::life_ticker{});

    w.spawn_actor([](entt::registry& reg, const entt::entity e) {
        atmosphere::make(reg, e);
    });

    {
        constexpr int ITEMS_COUNT = 150;
        std::array<pos, ITEMS_COUNT> all{};
        std::mt19937 gen(std::random_device{}());
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
        std::uniform_int_distribution<> dist_ch(printable_range.first,
                                                printable_range.second);
        for(auto i: all) {
            w.spawn_actor([&](entt::registry& reg, const entt::entity ent) {
                std::string s{' '};
                do {
                    s[0] = static_cast<char>(dist_ch(gen));
                } while(s[0] == '#' || s[0] == '-' || s[0] == '_');
                actor::make_base_renderable(
                    reg, ent, i, 3,
                    sprite(sprite::unchecked_construct_tag{}, s));
                reg.emplace<collision::agent>(ent);
                reg.emplace<collision::on_collide>(
                    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
                    ent, [](entt::registry& reg, const entt::entity self,
                            const entt::entity collider) {
                        if(reg.all_of<character>(collider)) {
                            char ascii_mesh = reg.get<sprite>(self).data()[0];
                            const auto l = reg.get<const loc>(self);
                            using namespace pos_declaration;
                            rng.seed(static_cast<int64_t>(
                                static_cast<int>(ascii_mesh) + l[Y] + l[X]));
                            size_t rand_msg = dist(rng);
                            notepad::push_command([msg = messages[rand_msg]](
                                                      notepad* np, scintilla*) {
                                SetWindowText(np->popup_window.window,
                                              msg.data());
                                np->popup_window.text = msg;
                            });
                        }
                        if(reg.all_of<projectile>(collider)) {
                            reg.emplace_or_replace<life::begin_die>(self);
                        }
                        return collision::responce::block;
                    });
                reg.emplace<life::death_last_will>(
                    ent, [](entt::registry& reg_, const entt::entity self) {
                        const auto dead = reg_.create();
                        auto old_loc = reg_.get<loc>(self);

                        actor::make_base_renderable(
                            reg_, dead, old_loc - loc(1, 0), 1,
                            sprite(sprite::unchecked_construct_tag{}, "___"));
                        reg_.emplace<life::lifetime>(dead,
                                                     std::chrono::seconds{3});
                        reg_.emplace<timeline::eval_direction>(dead);
                        reg_.emplace<timeline::what_do>(
                            dead, [](entt::registry& r_, const entt::entity e_,
                                     timeline::direction /**/) {
                                r_.get<translation>(e_).mark();
                            });
                    });
            });
        }
    }
#ifndef NDEBUG
    w.spawn_actor([](entt::registry& reg, const entt::entity e) {
        static constexpr pos gun_pos = pos{14, 20};
        gun::make(reg, e, gun_pos);
    });
#endif
    w.spawn_actor([](entt::registry& reg, const entt::entity ent) {
        reg.emplace<sprite>(ent,
                            sprite(sprite::unchecked_construct_tag{}, "#"));
        static constexpr pos char_pos = pos{20, 14};
        character::make(reg, ent, char_pos);
        auto& [input_callback] =
            reg.emplace<input::processor::down_signal>(ent);
        input_callback.connect(&character::process_movement_input<>);
    });
};
} // namespace game
