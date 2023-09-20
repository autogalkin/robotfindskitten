
#include <array>
#pragma warning(push, 0)
#include <Windows.h>
#pragma warning(pop)

#include "details/base_types.h"
#include "details/gamelog.h"
#include "ecs_processors/drawers.h"
#include "ecs_processors/input_passer.h"
#include "ecs_processors/killer.h"
#include "ecs_processors/motion.h"
#include "ecs_processors/screen_change_handler.h"
#include "ecs_processors/timers.h"
#include "engine.h"
#include "entities/factories.h"
#include "notepader.h"
#include "world.h"

namespace game {
inline void start() {
    const auto& w = notepader::get().get_engine()->get_world();
    auto& exec = w->get_ecs_executor();

    // the game tick pipeline of the ecs
    exec.push<screen_change_handler>(
        notepader::get().get_engine().get()); // TODO its only observer
    exec.push<input_passer>(notepader::get().get_input_manager().get());
    exec.push<uniform_motion>();
    exec.push<non_uniform_motion>();
    exec.push<timeline_executor>();
    exec.push<rotate_animator>();
    exec.push<collision::query>();
    exec.push<redrawer>();
    exec.push<death_last_will_executor>();
    exec.push<killer>();
    exec.push<lifetime_ticker>();

    w->spawn_actor([](entt::registry& reg, const entt::entity entity) {
        atmosphere::make(reg, entity);
    });

    {
        int i = 0;
        int j = 0;
        auto spawn_block = [&i, &j](entt::registry& reg,
                                    const entt::entity entity) {
            actor::make_base_renderable(
                reg, entity,
                {6 + static_cast<double>(i), 4 + static_cast<double>(j)}, 1,
                {shape::sprite::from_data{U"█", 1, 1}});
            reg.emplace<collision::agent>(entity);
            reg.emplace<collision::on_collide>(entity);
        };

        for (i = 0; i < 4; ++i) {
            ++j;
            w->spawn_actor(spawn_block);
        }
        // w->spawn_actor([](entt::registry& reg, const entt::entity entity){
        //     actor::make_base_renderable(reg, entity, {16, 24}, 2,
        //     {shape::sprite::from_data{U"Hello Interface!", 1, 15}});
        // });
        w->spawn_actor([](entt::registry& reg, const entt::entity entity) {
            actor::make_base_renderable(reg, entity, {2, 0}, 2,
                                        {shape::sprite::from_data{U"-", 1, 1}});
            reg.emplace<screen_resize>(entity, [&reg,
                                                entity](const uint32_t width,
                                                        const uint32_t height) {
                auto& sh = reg.get<shape::sprite_animation>(entity);
                sh.current_sprite() = shape::sprite{shape::sprite::from_data{
                    std::vector<char32_t>(width, U'—').data(), 1, width}};
            });
            reg.emplace<screen_scroll>(entity, [&reg, entity](
                                                   const position& new_scroll) {
                auto& [current, translation] = reg.get<location_buffer>(entity);
                translation.pin() = location{
                    0., static_cast<double>(new_scroll.index_in_line()) -
                            current.index_in_line()};
            });
        });
        w->spawn_actor([](entt::registry& reg, const entt::entity entity) {
            actor::make_base_renderable(
                reg, entity, {0, 0}, 2,
                {shape::sprite::from_data{U"It's a banana! Oh, joy!", 1, 23}});
            reg.emplace<screen_scroll>(entity, [&reg, entity](
                                                   const position& new_scroll) {
                auto& [current, translation] = reg.get<location_buffer>(entity);
                translation.pin() = location{
                    0., static_cast<double>(new_scroll.index_in_line()) -
                            current.index_in_line()};
            });
        });
    }

    w->spawn_actor([](entt::registry& reg, const entt::entity entity) {
        gun::make(reg, entity, {14, 20});
    });

    w->spawn_actor([](entt::registry& reg, const entt::entity entity) {
        reg.emplace<shape::sprite_animation>(
            entity,
            std::vector<shape::sprite>{
                {{shape::sprite::from_data{U"f-", 1, 2}},
                 {shape::sprite::from_data{U"-f", 1, 2}}}},
            static_cast<uint8_t>(0));

        character::make(reg, entity, location{3, 3});
        auto& [input_callback] = reg.emplace<input_passer::down_signal>(entity);
        input_callback.connect(&character::process_movement_input<>);
        character::add_top_down_camera(reg, entity);
    });

    w->spawn_actor([](entt::registry& reg, const entt::entity entity) {
        monster::make(reg, entity, {10, 3});
    });
};
} // namespace game
