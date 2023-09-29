#pragma once
#include <array>

#include <Windows.h>
#include <chrono>
#include <string>
#include <string_view>
#include <random>

#include "engine/details/base_types.h"

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
typedef std::mt19937 rng_type;
static std::uniform_int_distribution<rng_type::result_type>
    dist(0, messages.size() - 1);
static rng_type rng{};

static lexer game_lexer{};

inline void define_all_styles(scintilla* sc) {
    static_assert(int(' ') + 100 == 132);
    int style = 132;
    sc->dcall0(SCI_STYLECLEARALL);
    for (auto i : ALL_COLORS) {
        sc->dcall2(SCI_STYLESETFORE, style, i);
        style++;
    }
}

inline void start(world& w, back_buffer& buf, notepad::commands_queue_t& cmds,
                  const int game_area[2]) {

    cmds.push([](notepad*, scintilla* sc) {
        sc->set_lexer(&game_lexer);
        define_all_styles(sc);
    });

    auto& exec = w.processors;
    exec.push_back(input::processor{});
    exec.push_back(uniform_motion{});
    exec.push_back(non_uniform_motion{});
    exec.push_back(timeline_executor{});
    exec.push_back(rotate_animator{});
    exec.push_back(collision::query(w, game_area));
    exec.push_back(redrawer(buf, w));
    exec.push_back(death_last_will_executor{});
    exec.push_back(killer{});
    exec.push_back(lifetime_ticker{});

    w.spawn_actor([](entt::registry& reg, const entt::entity e) {
        atmosphere::make(reg, e);
    });

    {
        constexpr int ITEMS_COUNT = 150;
        std::array<pos, ITEMS_COUNT> all{};
        std::mt19937 gen(std::random_device{}());
        std::uniform_int_distribution<> dist_x(0, game_area[0] - 3 - 1);
        std::uniform_int_distribution<> dist_y(0, game_area[1] - 1 - 1);
        for (auto i = 0; i < ITEMS_COUNT; i++) {
            pos p;
            do {
                p = pos{dist_x(gen), dist_y(gen)};
            } while (
                std::ranges::any_of(all.begin(), all.begin() + i,
                                    [p](auto other) { return p == other; }));
            all[i] = p;
        }
        std::uniform_int_distribution<> dist_ch(32, 127);
        for (auto i : all) {
            w.spawn_actor([&](entt::registry& reg, const entt::entity ent) {
                char ch[2];
                do {
                    ch[0] = dist_ch(gen);
                } while (ch[0] == '#' || ch[0] == '-' || ch[0] == '_');
                ch[1] = '\0';

                actor::make_base_renderable(reg, ent, i, 3, sprite(ch));
                reg.emplace<collision::agent>(ent);
                reg.emplace<collision::on_collide>(
                    ent, [](entt::registry& reg, const entt::entity self,
                            const entt::entity collider) {
                        if (reg.all_of<character>(collider)) {
                            char ascii_mesh = reg.get<sprite>(self).data[0];
                            const auto l = reg.get<const loc>(self);
                            using namespace pos_declaration;
                            rng.seed(int(ascii_mesh) + l[Y] + l[X]);
                            size_t rand_msg = dist(rng);
                            notepad::push_command([msg = messages[rand_msg]](
                                                      notepad* np, scintilla*) {
                                SetWindowText(np->popup_window.window,
                                              msg.data());
                                np->popup_window.text = msg;
                            });
                        }
                        if (reg.all_of<projectile>(collider)) {
                            reg.emplace_or_replace<life::begin_die>(self);
                        }
                        return collision::responce::block;
                    });
                reg.emplace<life::death_last_will>(
                    ent, [](entt::registry& reg_, const entt::entity self) {
                        const auto dead = reg_.create();
                        auto old_loc = reg_.get<loc>(self);

                        actor::make_base_renderable(
                            reg_, dead, old_loc - loc(1, 0), 1, sprite("___"));
                        reg_.emplace<life::lifetime>(dead,
                                                     std::chrono::seconds{3});
                        reg_.emplace<timeline::eval_direction>(dead);
                        reg_.emplace<timeline::what_do>(
                            dead, [](entt::registry& r_, const entt::entity e_,
                                     timeline::direction d) {
                                r_.get<translation>(e_).mark();
                            });
                    });
            });
        }
    }
    w.spawn_actor([](entt::registry& reg, const entt::entity e) {
        gun::make(reg, e, {14, 20});
    });

    w.spawn_actor([](entt::registry& reg, const entt::entity ent) {
        reg.emplace<sprite>(ent, sprite("#"));

        character::make(reg, ent, loc(24, 7));
        auto& [input_callback] =
            reg.emplace<input::processor::down_signal>(ent);
        input_callback.connect(&character::process_movement_input<>);
    });
};
} // namespace game
