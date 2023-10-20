/**
 * @file
 * @brief The entry point to the game.
 */

#ifndef _CPP_PROJECTS_ROBOTFINDSKITTEN_SRC_ROBOTFINDSKITTEN_GAME_GAME_H
#define _CPP_PROJECTS_ROBOTFINDSKITTEN_SRC_ROBOTFINDSKITTEN_GAME_GAME_H

#include <array>
#include <chrono>
#include <random>
#include <span>
#include <string>
#include <string_view>
#include <unordered_set>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <glm/gtx/hash.hpp>
#include <entt/entity/registry.hpp>
#include <entt/entity/organizer.hpp>

#include <Windows.h>

#include "engine/buffer.h"
#include "engine/details/base_types.hpp"
#include "engine/notepad.h"
#include "engine/scintilla_wrapper.h"
#include "engine/time.h"
#include "game/systems/collision_query.h"
#include "game/systems/drawers.h"
#include "game/systems/input.h"
#include "game/systems/life.h"
#include "game/systems/motion.h"
#include "game/factories.h"
#include "game/lexer.h"
#include "messages.h"

namespace game {

/**
 * @var MESSAGES
 * @brief All `robotfindskitten` messages.
 *
 * They come from config.h.in and are embedded by CMake.
 **/
constexpr std::array MESSAGES =
    std::to_array<std::string_view>(ALL_GAME_MESSAGES);

// A thread safe buffer for this game
using buffer_type = back_buffer<thread_safe_trait<std::mutex>>;

void run(buffer_type& game_buffer);

/**
 * @brief Starts a game loop within the specified extends.
 *
 * @param game_area An area where the game can operate, including collision,
 * drawing, and more.
 * @param shutdown A shutdown signal to exit the loop.
 */
inline void start(pos game_area,
                  const std::shared_ptr<std::atomic_bool>& shutdown) {
    auto GAME_LEXER = lexer{};
    notepad::push_command(
        [&GAME_LEXER](notepad&, scintilla::scintilla_dll& sc) {
            scintilla::look_op{sc}.set_lexer(&GAME_LEXER);
        });
    auto game_buffer = back_buffer<thread_safe_trait<std::mutex>>{game_area};
    while(!shutdown->load()) {
        run(game_buffer);
        auto lock = game_buffer.lock();
        game_buffer.clear();
        // Restart a game again
    }
};

void setup_components(entt::registry& reg, buffer_type& game_buffer);
void game_loop(entt::registry& reg, entt::organizer org,
               buffer_type& game_buffer);

/**
 * @brief Sends the content of the \p game_buffer to Scintilla.
 *
 * @param game_buffer A buffer for rendering.
 */
inline void render(buffer_type& game_buffer) {
    notepad::push_command(
        [&game_buffer](notepad& /*np*/, scintilla::scintilla_dll& sc) {
            // swaps the buffer in Scintilla
            const auto caret = scintilla::caret_op{sc};
            const auto pos = caret.get_caret_index();
            game_buffer.view([&sc](const auto& buf) {
                scintilla::text_op{sc}.set_new_all_text(buf);
            });
            caret.set_caret_index(pos);
        });
}

/**
 * @brief Initializes the systems execution graph and runs a game loop.
 *
 * @param game_buffer A back buffer of the current game.
 */
inline void run(buffer_type& game_buffer) {
    entt::registry reg; // Acts as static variable for the game.
    // All systems live on the stack until the game ends.
    auto run_with = [&reg, &game_buffer]<typename... Systems>(Systems... t) {
        entt::organizer org;
        auto system_names = std::to_array(
            {boost::typeindex::type_id<Systems>().pretty_name()...});
        {
            size_t i = 0;
            (
                [&]() {
                    org.emplace<&decltype(t)::operator()>(
                        t, system_names.at(i++).c_str());
                }(),
                ...);
        }
        // Fills the initial area inside Scintilla
        // It allows us to scroll to the random start point.
        render(game_buffer);
        setup_components(reg, game_buffer);
        game_loop(reg, std::move(org), game_buffer);
    };
    // The execution graph.
    // The order of systems is important.For example, the collision system
    // checks a future position of an entity based on its translation, which is
    // set by the motion system.
    // clang-format off
    run_with(input::player_input{reg}, 
             motion::uniform_motion{}, 
             motion::non_uniform_motion{},
             timeline::timeline_step{reg}, 
             drawing::rotate_animator{},
             collision::query{reg, game_buffer.get_extends()}, 
             drawing::check_need_update{},
             drawing::lock_buffer<buffer_type>{game_buffer, reg},
             drawing::erase_buffer<buffer_type>{game_buffer}, 
             drawing::apply_translation{},
             drawing::redraw<buffer_type>{game_buffer, reg}, 
             drawing::cleanup<buffer_type>{},
             life::fullfill_dying_wish{reg}, 
             life::killer{reg},
             life::life_ticker{});
    // clang-format on
}

void print_graph(std::span<const entt::organizer::vertex> graph);

/**
 * @brief Executes the systems graph following a fixed time step.
 *
 * @param reg An entt::registry with all game components.
 * @param org An entt::organizer with all systems.
 * @param game_buffer Our game's back buffer.
 */
inline void game_loop(entt::registry& reg, entt::organizer org,
                      buffer_type& game_buffer) {
    const auto graph = org.graph();
    org.clear();
#ifndef NDEBUG
    print_graph(graph);
#endif

    timings::fixed_time_step fixed_time_step{};
    timings::fps_count fps_count{};
    auto& game_flag = reg.ctx().emplace<game_over::game_status_flag>(
        game_over::game_status_flag::unset);
    auto& dt = reg.ctx().emplace<timings::duration>(timings::dt);

    for(const auto& vertex: graph) {
        vertex.prepare(reg);
    }

    while(game_flag == game_over::game_status_flag::unset) {
        dt = fixed_time_step.sleep();
        fps_count.fps([](auto fps) {
            notepad::push_command(
                [fps](notepad& np, scintilla::scintilla_dll&) {
                    np.send_game_fps(fps);
                });
        });
        // Execute all systems.
        for(const auto& vertex: graph) {
            // The delta time is implicitly passed through ctx().
            vertex.callback()(vertex.data(), reg);
        }
        render(game_buffer);
    }
}

constexpr size_t ITEMS_COUNT = MESSAGES.size();

inline std::unordered_set<pos> generate_random_positions(entt::registry& reg,
                                                         pos game_area) {
    std::unordered_set<pos> all{};
    all.reserve(ITEMS_COUNT + 3);
    auto& rnd = reg.ctx().get<random_t>();
    std::uniform_int_distribution<> dist_x(0, game_area.x - 1 - 1);
    std::uniform_int_distribution<> dist_y(0, game_area.y - 1);

    for(auto i = 0; i < ITEMS_COUNT + 3; i++) {
        pos p;
        do {
            p = pos{dist_x(rnd), dist_y(rnd)};
        } while(all.contains(p));
        all.insert(p);
    }
    return all;
}

/**
 * @struct message_index
 * @brief An index in \ref MESSAGES
 */
struct message_index {
    size_t i;
};

inline void
distribute_items(entt::registry& reg,
                 std::uniform_int_distribution<> random_mesh_dist,
                 std::unordered_set<pos>::const_iterator pos_iter,
                 const std::unordered_set<pos>::const_iterator& end) {
    auto generate_array =
        []<size_t... Indices>(std::integer_sequence<size_t, Indices...>)
        -> std::array<size_t, sizeof...(Indices)> {
        return {(static_cast<size_t>(Indices))...};
    };
    auto rand_msgs =
        generate_array(std::make_integer_sequence<size_t, MESSAGES.size()>{});
    auto rnd = reg.ctx().get<random_t>();
    std::shuffle(rand_msgs.begin(), rand_msgs.end(), rnd);
    for(size_t i = 0; pos_iter != end; ++pos_iter) {
        auto item_pos = *pos_iter;
        auto item = entt::handle{reg, reg.create()};
        std::string s{' '};
        do {
            s[0] = static_cast<char>(random_mesh_dist(rnd));
        } while(std::ranges::any_of(std::to_array({'#', '-', '_'}),
                                    [&s](char x) { return x == s[0]; }));
        factories::make_base_renderable(
            item, item_pos, 3, sprite(sprite::unchecked_construct_tag{}, s));

        item.emplace<message_index>(rand_msgs.at(i++));
        item.emplace<collision::agent>();
        item.emplace<collision::hit_extends>(item.get<sprite>().bounds());
        item.emplace<collision::responce_func>(
            +[](const void*, entt::registry& reg, collision::self self,
                collision::collider collider) {
                if(reg.all_of<character::character_tag>(collider)) {
                    auto msg_wnd_uuid =
                        reg.ctx().get<static_control_handler::id_t>(
                            entt::hashed_string{"msg_window"});
                    notepad::push_command(
                        [msg_wnd_uuid,
                         msg =
                             MESSAGES.at(reg.get<const message_index>(self).i)](
                            notepad& np, scintilla::scintilla_dll&) {
                            auto w = std::ranges::find_if(
                                np.get_all_static_controls(),
                                [msg_wnd_uuid](auto& w) {
                                    return w.get_id() == msg_wnd_uuid;
                                });
                            ::SetWindowText(*w, msg.data());
                            w->with_text(msg);
                        });
                }
                if(reg.all_of<projectile::projectile_tag>(collider)) {
                    reg.emplace_or_replace<life::begin_die>(self);
                }
            });
        factories::emplace_simple_death_anim(item);
    }
}

inline void create_gun(entt::registry& reg, [[maybe_unused]] pos gun_pos,
                       [[maybe_unused]] char gun_mesh) {
    auto gun = entt::handle{reg, reg.create()};
#ifndef NDEBUG
    gun::make(gun, pos{14, 20}, sprite(sprite::unchecked_construct_tag{}, "<"));
#else
    gun::make(gun, gun_pos,
              sprite(sprite::unchecked_construct_tag{}, std::string{gun_mesh}));
#endif
}
inline void create_character([[maybe_unused]] pos char_pos, entt::registry& reg,
                             [[maybe_unused]] pos game_area) {
    auto ch = entt::handle{reg, reg.create()};
    reg.ctx().emplace_as<entt::entity>(entt::hashed_string{"character_id"},
                                       ch.entity());
    ch.emplace<input::key_down_task>(input::key_down_task::function_type{
        &character::process_movement_input<>});
#ifndef NDEBUG
    character::make(ch, pos{10, 25},
                    sprite{sprite::unchecked_construct_tag{}, "#"});
#else
    character::make(ch, char_pos,
                    sprite{sprite::unchecked_construct_tag{}, "#"});
    notepad::push_command(
        [char_pos, game_area](notepad&, scintilla::scintilla_dll& sc) {
            auto scroll_op = scintilla::scroll_op{sc};
            auto size_op = scintilla::size_op{sc};
            npi_t height = size_op.get_lines_on_screen();
            auto width = static_cast<npi_t>(size_op.get_window_width()
                                            / size_op.get_char_width());
            scroll_op.scroll(
                std::max(0, static_cast<npi_t>(std::min(char_pos.x - width / 2,
                                                        game_area.x - width))),
                static_cast<npi_t>(
                    std::min(char_pos.y - height / 2, game_area.y - height)));
        });
#endif
}

inline void create_kitten(entt::registry& reg, [[maybe_unused]] pos kitten_pos,
                          [[maybe_unused]] char kitten_mesh) {
    auto kitten = entt::handle{reg, reg.create()};
#ifndef NDEBUG
    kitten::make(kitten, pos{10, 30},
                 sprite(sprite::unchecked_construct_tag{}, "Q"));
#else
    kitten::make(
        kitten, kitten_pos,
        sprite(sprite::unchecked_construct_tag{}, std::string{kitten_mesh}));
#endif
}

using rgb_t = decltype(RGB(0, 0, 0));

inline void set_all_colors(scintilla::scintilla_dll& sc,
                           std::span<const rgb_t> all_colors) {
    int style = MY_STYLE_START + PRINTABLE_RANGE.first;
    const auto look_op = scintilla::look_op{sc};
    look_op.clear_all_styles();
    for(auto i: all_colors) {
        look_op.set_text_color(style, i);
        look_op.force_set_back_color(style, 0);
        style++;
    }
    // Default styles
    for(auto i: {0, STYLE_DEFAULT}) {
        look_op.force_set_back_color(i, 0);
        look_op.force_set_text_color(i, RGB(255, 255, 255));
    }
};

inline rgb_t hsl2rgb(double h, double s, double l) {
    double v = (l <= 0.5) ? (l * (1.0 + s)) : (l + s - l * s);
    auto to_rgb = [](double to_r, double to_g, double to_b) {
        return RGB(to_r * 255, to_g * 255, to_b * 255);
    };
    if(v > 0) {
        double m = l + l - v;
        double sv = (v - m) / v;
        h *= 6.0;
        auto sextant = static_cast<int32_t>(h);
        double fract = h - sextant;
        double vsf = v * sv * fract;
        double mid1 = m + vsf;
        double mid2 = v - vsf;
        switch(sextant) {
        case 0:
            return to_rgb(v, mid1, m);
        case 1:
            return to_rgb(mid2, v, m);
        case 2:
            return to_rgb(m, v, mid1);
        case 3:
            return to_rgb(m, mid2, v);
        case 4:
            return to_rgb(mid1, m, v);
        case 5:
            return to_rgb(v, m, mid2);
        }
    }
    return to_rgb(l, l, l);
}

inline std::vector<rgb_t> generate_colors(random_t& rnd) {
    std::uniform_real_distribution<> distr_h(0., 1.);
    std::uniform_real_distribution<> distr_s(0., 1.);
    std::uniform_real_distribution<> distr_l(0.4, 1.);
    auto all = std::vector<rgb_t>();
    all.resize(PRINTABLE_RANGE.second - PRINTABLE_RANGE.first + 1);
    for(auto& i: all) {
        i = hsl2rgb(distr_h(rnd), distr_s(rnd), distr_l(rnd));
    }
    return all;
}

inline void setup_components(entt::registry& reg, buffer_type& game_buffer) {
    auto& rnd = reg.ctx().emplace<random_t>(std::random_device{}());
    notepad::push_command([ALL_COLORS = generate_colors(rnd)](
                              notepad&, scintilla::scintilla_dll& sc) {
        set_all_colors(sc, ALL_COLORS);
    });
    // A common data is shared among all projectile instances, such as a single
    // sprite
    projectile::initialize_for_all(reg);

    auto all = generate_random_positions(reg, game_buffer.get_extends());
    auto end = std::next(all.end(), -3);
    std::uniform_int_distribution<> dist_char(PRINTABLE_RANGE.first + 1,
                                              PRINTABLE_RANGE.second);
    distribute_items(reg, dist_char, all.begin(), std::next(all.end(), -3));
    create_character(*end, reg, game_buffer.get_extends());
    std::advance(end, 1);
    create_gun(reg, *end, static_cast<char>(dist_char(rnd)));
    std::advance(end, 1);
    create_kitten(reg, *end, static_cast<char>(dist_char(rnd)));

    auto msg_w = static_control_handler{};
    reg.ctx().emplace_as<static_control_handler::id_t>(
        entt::hashed_string{"msg_window"}, msg_w.get_id());
    notepad::push_command([msg_w = std::move(msg_w)](
                              notepad& np,
                              scintilla::scintilla_dll& sc) mutable {
        static constexpr int height = 100;
        static constexpr int wnd_x = 20;
        msg_w.with_size(pos(scintilla::size_op{sc}.get_window_width(), height))
            .with_position(pos(wnd_x, 0))
            .with_fore_color(
                scintilla::look_op{sc}.get_text_color(STYLE_DEFAULT));
        np.show_static_control(std::move(msg_w));
    });
};

// Taken from the entt community
// https://github.com/fowlmouth/entt-execution-graph/
inline void print_graph(std::span<const entt::organizer::vertex> graph) {
    std::cout << "the game execution graph:\n";
    std::vector<const entt::type_info*> typeinfo_buffer;
    typeinfo_buffer.reserve(64);
    for(const auto& vert: graph) {
        auto ro_count = vert.ro_count();
        auto rw_count = vert.rw_count();

        std::cout << "vert name= " << (vert.name() ? vert.name() : "(no name)")
                  << std::endl;

        std::cout << "  ro_count= " << ro_count << std::endl;
        typeinfo_buffer.resize(ro_count);
        vert.ro_dependency(typeinfo_buffer.data(), typeinfo_buffer.size());
        for(const auto* typeinfo: typeinfo_buffer) {
            std::cout << "    " << typeinfo->name() << std::endl;
        }

        std::cout << "  rw_count= " << rw_count << std::endl;
        typeinfo_buffer.resize(rw_count);
        vert.rw_dependency(typeinfo_buffer.data(), typeinfo_buffer.size());
        for(const auto* typeinfo: typeinfo_buffer) {
            std::cout << "    " << typeinfo->name() << std::endl;
        }

        std::cout << "  is_top_level= " << (vert.top_level() ? "yes" : "no")
                  << std::endl
                  << "  children count= " << vert.children().size()
                  << std::endl;
        for(const size_t child: vert.children()) {
            std::cout << "    "
                      << (graph[child].name() ? graph[child].name()
                                              : "(noname)")
                      << std::endl;
        }
        std::cout << std::endl;
    }
}

} // namespace game

#endif
