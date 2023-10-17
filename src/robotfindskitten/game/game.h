#ifndef _CPP_PROJECTS_ROBOTFINDSKITTEN_SRC_ROBOTFINDSKITTEN_GAME_GAME_H
#define _CPP_PROJECTS_ROBOTFINDSKITTEN_SRC_ROBOTFINDSKITTEN_GAME_GAME_H
/**
 * @file
 * @brief The main game loop
 */

#include <array>
#include <chrono>
#include <random>
#include <span>
#include <string>
#include <string_view>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <entt/entity/fwd.hpp>
#include <glm/gtx/hash.hpp>

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

// All `robotfindskitten` messages
static const std::array MESSAGES =
    std::to_array<std::string_view>(ALL_GAME_MESSAGES);

// Using thread safe buffer
using buffer_type = back_buffer<thread_safe_trait<std::mutex>>;

inline void run(buffer_type& game_buffer);

/**
 * @brief Start a game loop
 *
 * @param game_area An area where game can work(collision, draw ent)
 * @param shutdown A shutdown signal
 */
inline void start(pos game_area, std::shared_ptr<std::atomic_bool> shutdown) {
    lexer GAME_LEXER{};
    notepad::push_command(
        [&GAME_LEXER](notepad&, scintilla& sc) { sc.set_lexer(&GAME_LEXER); });
    auto game_buffer = back_buffer<thread_safe_trait<std::mutex>>{game_area};
    while(!shutdown->load()) {
        run(game_buffer);
        auto lock = game_buffer.lock();
        game_buffer.clear();
        // restart a game
    }
};

inline void setup_components(entt::registry& reg, buffer_type& game_buffer);
inline void game_loop(entt::registry& reg, entt::organizer& org,
                      buffer_type& game_buffer);

/**
 * @brief Initialize systems and run a game loop
 *
 * @param game_buffer A game back buffer
 */
inline void run(buffer_type& game_buffer) {
    entt::registry reg;
    // all systems live on the stack until the game end
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
        setup_components(reg, game_buffer);
        game_loop(reg, org, game_buffer);
        org.clear();
    };
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

/**
 * @brief Game loop that execute the systems graph in the fixed time step
 *
 * @param reg entt registry
 * @param org entt organizer
 * @param game_buffer our game back buffer
 */
inline void game_loop(entt::registry& reg, entt::organizer& org,
                      buffer_type& game_buffer) {
    const auto graph = org.graph();

    timings::fixed_time_step fixed_time_step{};
    timings::fps_count fps_count{};
    auto& game_flag = reg.ctx().emplace<game_over::game_status_flag>(
        game_over::game_status_flag::unset);
    auto& dt = reg.ctx().emplace<timings::duration>(timings::dt);

    for(auto& vertex: graph) {
        vertex.prepare(reg);
    }

    while(game_flag == game_over::game_status_flag::unset) {
        fixed_time_step.sleep();
        fps_count.fps([](auto fps) {
            notepad::push_command([fps](notepad& np, scintilla&) {
                np.window_title.game_thread_fps =
                    static_cast<decltype(np.window_title.game_thread_fps)>(
                        fps);
            });
        });

        // TODO(Igor): real alpha
        static_cast<void>(dt);
        for(const auto& vertex: graph) {
            vertex.callback()(vertex.data(), reg);
        }

        notepad::push_command([&game_buffer](notepad& /*np*/, scintilla& sc) {
            // swap buffers in Scintilla
            const auto pos = sc.get_caret_index();
            game_buffer.view(
                [sc](const auto& buf) { sc.set_new_all_text(buf); });
            sc.set_caret_index(pos);
        });
    }
}

inline static constexpr int ITEMS_COUNT = 48;
static_assert(MESSAGES.size() >= ITEMS_COUNT
              && "Count of messages cannon be less than items");

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

// index in MESSAGES
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
                            notepad& np, scintilla&) {
                            auto w = std::ranges::find_if(
                                np.static_controls, [msg_wnd_uuid](auto& w) {
                                    return w.get_id() == msg_wnd_uuid;
                                });
                            SetWindowText(*w, msg.data());
                            w->text = msg;
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
inline void create_character(entt::registry& reg, [[maybe_unused]] pos char_pos,
                             [[maybe_unused]] pos game_area) {
    auto ch = entt::handle{reg, reg.create()};
    reg.ctx().emplace_as<entt::entity>(entt::hashed_string{"character_id"},
                                       ch.entity());
#ifndef NDEBUG
    character::make(ch, pos{10, 25},
                    sprite{sprite::unchecked_construct_tag{}, "#"});
#else
    character::make(ch, char_pos,
                    sprite{sprite::unchecked_construct_tag{}, "#"});
    notepad::push_command([char_pos, game_area](notepad&, scintilla& sc) {
        auto height = sc.get_lines_on_screen();
        auto width = sc.get_window_width() / sc.get_char_width();
        sc.scroll(std::max(0, static_cast<int>(std::min(char_pos.x - width / 2,
                                                         game_area.x - width))),
                   std::max(0, std::min(char_pos.y - height / 2,
                                        game_area.y - height)));
    });
#endif
    ch.emplace<input::key_down_task>(input::key_down_task::function_type{
        &character::process_movement_input<>});
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

inline void define_all_styles(scintilla& sc,
                              std::span<const color_t> all_colors) {
    int style = MY_STYLE_START + PRINTABLE_RANGE.first;
    sc.clear_all_styles();
    for(auto i: all_colors) {
        sc.set_text_color(style, i);
        style++;
    }
}

static inline constexpr int COLOR_COUNT = 255;
using color_t = decltype(RGB(0, 0, 0));
using all_colors_t =
    std::array<color_t, PRINTABLE_RANGE.second - PRINTABLE_RANGE.first - 1>;
inline all_colors_t generate_colors(random_t& rnd) {
    std::uniform_int_distribution<> distr(0, COLOR_COUNT);
    auto arr = std::array<decltype(RGB(0, 0, 0)),
                          PRINTABLE_RANGE.second - PRINTABLE_RANGE.first - 1>();
    for(auto& i: arr) {
        i = RGB(distr(rnd), distr(rnd), distr(rnd));
    }
    return arr;
}

inline void setup_components(entt::registry& reg, buffer_type& game_buffer) {
    auto& rnd = reg.ctx().emplace<random_t>(std::random_device{}());
    const all_colors_t ALL_COLORS = generate_colors(rnd);
    notepad::push_command([&ALL_COLORS](notepad&, scintilla& sc) {
        define_all_styles(sc, ALL_COLORS);
    });
    projectile::initialize_for_all(reg);
    atmosphere::make(entt::handle{reg, reg.create()});

    auto all = generate_random_positions(reg, game_buffer.get_extends());
    auto end = std::next(all.end(), -3);
    std::uniform_int_distribution<> dist_char(PRINTABLE_RANGE.first + 1,
                                              PRINTABLE_RANGE.second);
    distribute_items(reg, dist_char, all.begin(), std::next(all.end(), -3));
    create_character(reg, *end, game_buffer.get_extends());
    std::advance(end, 1);
    create_gun(reg, *end, static_cast<char>(dist_char(rnd)));
    std::advance(end, 1);
    create_kitten(reg, *end, static_cast<char>(dist_char(rnd)));

    auto msg_w = static_control_handler{};
    reg.ctx().emplace_as<static_control_handler::id_t>(
        entt::hashed_string{"msg_window"}, msg_w.get_id());
    notepad::push_command(
        [msg_w = std::move(msg_w)](notepad& np, scintilla& sc) mutable {
            static constexpr int height = 100;
            static constexpr int wnd_x = 20;
            msg_w.with_size(pos(sc.get_window_width(), height))
                .with_position(pos(wnd_x, 0))
                .with_text_color(sc.get_text_color(STYLE_DEFAULT));
            np.show_static_control(std::move(msg_w));
        });
};
} // namespace game

#endif
