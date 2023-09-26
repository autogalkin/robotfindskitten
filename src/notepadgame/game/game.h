#pragma once
#include <array>

#include <Windows.h>
#include <chrono>
#include <string>
#include <string_view>

#include "engine/details/base_types.h"

#include "engine/time.h"
#include "game/ecs_processors/drawers.h"
#include "game/ecs_processors/input.h"
#include "game/ecs_processors/killer.h"
#include "game/ecs_processors/motion.h"
#include "game/ecs_processors/timers.h"
#include "game/ecs_processors/render_commands.h"
#include "engine/scintilla_wrapper.h"
#include "game/entities/factories.h"
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

inline void define_all_styles(scintilla* sc) {
    static_assert(int(' ') + 100 == 132);
    int style = 132;
    // TODO
    sc->dcall0(SCI_STYLECLEARALL);
    for (auto i : ALL_COLORS) {
        sc->dcall2(SCI_STYLESETITALIC, style, 0); // italic
        sc->dcall2(SCI_STYLESETFORE, style, i);
        style++;
    }
}
struct is_message_area {};
static lexer game_lexer{};
inline void start(world& w, notepad::commands_queue_t& cmds,
                  const int game_area[2]) {

    cmds.push([](notepad*, scintilla* sc) {
        printf("Set a lexer\n");
        sc->set_lexer(&game_lexer);
        define_all_styles(sc);
        sc->dcall2(SCI_STYLESETFORE, 228, RGB(255, 0, 0));
    });

    auto& exec = w.executor;
    exec.push<input::processor>(&w);
    exec.push<uniform_motion>(&w);
    exec.push<non_uniform_motion>(&w);
    exec.push<timeline_executor>(&w);
    exec.push<rotate_animator>(&w);
    exec.push<collision::query>(&w, game_area);
    exec.push<redrawer>(&w);
    // exec.push<render_commands>(&w, &cmds);
    exec.push<death_last_will_executor>(&w);
    exec.push<killer>(&w);
    exec.push<lifetime_ticker>(&w);
    w.spawn_actor([](entt::registry& reg, const entt::entity entity) {
        reg.emplace<timeline::what_do>(
            entity, std::move([fps_count = timings::fps_count{}](
                                  entt::registry& reg, const entt::entity e,
                                  const direction) mutable {
                fps_count.fps([&reg, e](auto fps) {
                    notepad::push_command([fps](notepad* np, scintilla*) {
                        np->window_title.game_thread_fps = fps;
                    });
                });
            }));
        reg.emplace<timeline::eval_direction>(entity, direction::forward);
        // reg.emplace<notepad_thread_command>(entity);
    });
    w.spawn_actor([](entt::registry& reg, const entt::entity entity) {
        atmosphere::make(reg, entity);
    });
    // Message Area
    //
    entt::entity message_area;
    w.spawn_actor([&message_area](entt::registry& reg, const entt::entity entity) {
        actor::make_base_renderable(reg, entity, {1, 0}, 3, {});
        reg.emplace<is_message_area>(entity);
        reg.get<shape::sprite_animation>(entity).sprts.push_back({});
        message_area = entity;
    });
    {
        constexpr int ITEMS_COUNT = 150;
        std::array<position_t, ITEMS_COUNT> all{};
        std::mt19937 gen(std::random_device{}());
        std::uniform_int_distribution<> dist_x(0, game_area[0] - 3 - 1);
        std::uniform_int_distribution<> dist_y(0, game_area[1] - 1 - 1);
        for (auto i = 0; i < ITEMS_COUNT; i++) {
            position_t p;
            do {
                p = position_t{dist_x(gen), dist_y(gen)};
            } while (
                std::ranges::any_of(all.begin(), all.begin() + i,
                                    [p](auto other) { return p == other; }));
            all[i] = p;
        }
        std::uniform_int_distribution<> dist_ch(32, 127);
        for (auto i : all) {
            w.spawn_actor([&](entt::registry& reg, const entt::entity entity) {
                char ch[2];
                do {
                    ch[0] = dist_ch(gen);
                } while (ch[0] == '#' || ch[0] == '-');
                ch[1] = '\0';
                actor::make_base_renderable(
                    reg, entity,
                    {static_cast<double>(i.line() + 3),
                     static_cast<double>(i.index_in_line())},
                    3, {shape::sprite::from_data{ch, 1, 1}});
                reg.emplace<collision::agent>(entity);
                reg.emplace<collision::on_collide>(
                    entity, [message_area](entt::registry& reg, const entt::entity self,
                               const entt::entity collider) {
                        // TODO signals? Why all_of?
                        if(reg.all_of<character>(collider)){
                             char ascii_mesh =
                                reg.get<shape::sprite_animation>(self)
                                    .current_sprite()
                                    .data(0);
                            auto loc = reg.get<location_buffer>(self).current;
                            rng.seed(int(ascii_mesh)+loc.line()+loc.index_in_line());
                            size_t rand_msg = dist(rng);
                            auto& anim = reg.get<shape::sprite_animation>(message_area);
                            anim.rendering_i = 1 - anim.rendering_i;
                            anim.sprts[anim.rendering_i] =
                                shape::sprite(shape::sprite::from_data{
                                    messages[rand_msg].data(), 1,
                                    static_cast<npi_t>(
                                        messages[rand_msg].size())});
                            
                            reg.get<location_buffer>(
                                message_area).translation.mark();
                        }
                        return collision::on_collide::block_always(reg, self,
                                                                   collider);
                    });
            });
        }
    }

    /*
    {
        const auto frame = std::vector<char_size>(game_area[1], '_');
        w.spawn_actor([&frame, game_area](entt::registry& reg,
                                          const entt::entity entity) {
            actor::make_base_renderable(
                reg, entity, {3, 0}, 4,
                {shape::sprite::from_data{frame.data(), 1, game_area[1]}});
        });
    }

    {
        int i = 0;
        int j = 0;
        auto spawn_block = [&i, &j](entt::registry& reg,
                                    const entt::entity entity) {
            actor::make_base_renderable(
                reg, entity,
                {6 + static_cast<double>(i), 4 + static_cast<double>(j)}, 3,
                {shape::sprite::from_data{"]", 1, 1}});
            reg.emplace<collision::agent>(entity);
            reg.emplace<collision::on_collide>(entity);
        };

        for (i = 0; i < 4; ++i) {
            ++j;
            w.spawn_actor(spawn_block);
        }
    }
    */

    w.spawn_actor([](entt::registry& reg, const entt::entity entity) {
        gun::make(reg, entity, {14, 20});
    });

    w.spawn_actor([](entt::registry& reg, const entt::entity entity) {
        reg.emplace<shape::sprite_animation>(
            entity,
            std::vector<shape::sprite>{{{shape::sprite::from_data{"#", 1, 1}},
                                        {shape::sprite::from_data{"#", 1, 1}}}},
            static_cast<uint8_t>(0));

        character::make(reg, entity, location{7, 24});
        auto& [input_callback] =
            reg.emplace<input::processor::down_signal>(entity);
        input_callback.connect(&character::process_movement_input<>);
    });

    w.spawn_actor([](entt::registry& reg, const entt::entity entity) {
        monster::make(reg, entity, {10, 3});
    });
};
} // namespace game
