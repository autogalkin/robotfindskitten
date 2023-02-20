#pragma once
#include <entt/entt.hpp>
#include "../core/base_types.h"
#include "../ecs_processors/collision.h"
#include "../core/input.h"

struct projectile final
{
    static void make(entt::registry& reg, const entt::entity e
        , location start
        , velocity dir
        ,  const std::chrono::milliseconds life_time=std::chrono::seconds{1})
    {
        reg.emplace< shape::sprite>(e,  shape::sprite::initializer_from_data{"-", 1, 1});
        reg.emplace<shape::render_direction>(e);
        reg.emplace<shape::inverse_sprite>(e, shape::sprite::initializer_from_data{"-", 1, 1});
        
        reg.emplace<location_buffer>(e, std::move(start), dirty_flag<location>{});
        reg.emplace<non_uniform_movement_tag>(e);
        reg.emplace<velocity>(e, dir);
        reg.emplace<collision::agent>(e);
        
        reg.emplace<lifetime>(e, life_time);
        reg.emplace<death_last_will>(e, [](entt::registry& reg_, const entt::entity ent)
        {
            const auto& [current_proj_location, translation] = reg_.get<location_buffer>(ent);
            
            const auto new_e = reg_.create();
            reg_.emplace<location_buffer>(new_e, current_proj_location, dirty_flag<location>{translation});
            reg_.emplace<shape::sprite>( new_e, shape::sprite::initializer_from_data{"*", 1, 1});
            reg_.emplace<shape::render_direction>(new_e);
            reg_.emplace<shape::inverse_sprite>(new_e, shape::sprite::initializer_from_data{"*", 1, 1});
            reg_.emplace<lifetime>(new_e, std::chrono::seconds{1});
        });
        
    }
};

namespace timeline
{
    static void make(entt::registry& reg, const entt::entity e
        , std::function<void(entt::registry& , entt::entity, direction)> what_do  // NOLINT(performance-unnecessary-value-param)
        , const std::chrono::milliseconds duration=std::chrono::seconds{1}
        , direction dir     = direction::forward
 )
    {
        reg.emplace<lifetime>(e, duration);
        reg.emplace<timeline::what_do>(e, std::move(what_do));
        reg.emplace<timeline::eval_direction>(e, dir);
        
    }
};

// waits for the end of time and call a given function
struct timer final
{
    static void make(entt::registry& reg, const entt::entity e
        , std::function<void(entt::registry&, entt::entity)> what_do
        , const std::chrono::milliseconds duration=std::chrono::seconds{1})
    {
        reg.emplace<lifetime>(e, duration);
        reg.emplace<death_last_will>(e, std::move(what_do));
    }
};

struct character final
{
    static void make(entt::registry& reg, const entt::entity e, location l)
    {
        reg.emplace<location_buffer>(e, std::move(l), dirty_flag<location>{});
        reg.emplace<uniform_movement_tag>(e);
        reg.emplace<velocity>(e);
        reg.emplace<collision::agent>(e);
        
    }
    
    template< input::key UP   =input::key::w
            , input::key LEFT =input::key::a
            , input::key DOWN =input::key::s
            , input::key RIGHT=input::key::d
            , input::key ACTION=input::key::space
    >
    static void process_input(entt::registry& reg, const entt::entity e, const input::key_state_type& state)
    {
        auto& vel = reg.get<velocity>(e);
       
        for(auto& key : state)
        {
            switch (key)
            {
            case UP: vel.line()             -= 1; break;
            case LEFT: vel.index_in_line()  -= 1; break;
            case DOWN: vel.line()           += 1; break;
            case RIGHT: vel.index_in_line() += 1; break;
            case ACTION:
                {
                    auto& loc = reg.get<location_buffer>(e);
                    auto& sh = reg.get<shape::sprite>(e);
                    auto [dir] = reg.get<shape::render_direction>(e);
                    
                    const auto proj = reg.create();
                    location spawn_translation =  dir == direction::forward ? location{0, static_cast<double>(sh.bound_box().size.index_in_line() )} : location{0, -1};
                    projectile::make(reg, proj,loc.current + spawn_translation,  velocity{0, 15 * static_cast<float>(dir)}, std::chrono::seconds{4});
                    
                }
            default: break;
            }
        }
    }
};

struct atmosphere final
{
    static void make(entt::registry& reg, const entt::entity e)
    {
        timer::make(reg, e, &atmosphere::run_cycle, time_between_cycle);
        reg.emplace<timeline::eval_direction>(e, direction::reverse);
    }
    static void run_cycle(entt::registry& reg, const entt::entity timer)
    {
        // TODO таймер новый появляется когда другой еще не умер вроде
        const auto cycle_timeline = reg.create();

        
        timeline::make(reg, cycle_timeline,  &atmosphere::update_cycle, cycle_duration, reg.get<timeline::eval_direction>(timer).value);
        
        reg.emplace<color_range>(cycle_timeline);
        reg.emplace<death_last_will>(cycle_timeline, [](entt::registry& reg_, const entt::entity cycle_timeline_){
            
            const auto again_timer = reg_.create();
            timer::make(reg_, again_timer, &atmosphere::run_cycle, time_between_cycle);
            reg_.emplace<timeline::eval_direction>(again_timer,  direction_converter::invert(reg_.get<timeline::eval_direction>(cycle_timeline_).value));
            
        });
        
    }
    static void update_cycle(entt::registry& reg, const entt::entity e, const direction d)
    {
        const auto& [start, end] = reg.get<color_range>(e);
        const auto& [current_lifetime] = reg.get<lifetime>(e);
        
        //new_value = ( (old_value - old_min) / (old_max - old_min) ) * (new_max - new_min) + new_min
        double value = (current_lifetime - cycle_duration) / (std::chrono::duration<double>{0} - cycle_duration);
        value *= static_cast<double>(d);
        
        const COLORREF new_back_color = RGB(
                                        std::lerp(GetRValue(start), GetRValue(end), value)
                                      , std::lerp(GetGValue(start), GetGValue(end), value)
                                      , std::lerp(GetBValue(start), GetBValue(end), value)
                                      );
    
        const COLORREF new_front_color = RGB(
                                        std::lerp(GetRValue(start), GetRValue(end), -value)
                                      , std::lerp(GetGValue(start), GetGValue(end), -value)
                                      , std::lerp(GetBValue(start), GetBValue(end), -value)
                                      );
    
        notepader::get().get_engine()->force_set_background_color(new_back_color);
        notepader::get().get_engine()->force_set_all_text_color(new_front_color);
        
    }
private:
    inline static auto time_between_cycle = std::chrono::seconds{20};
    inline static auto cycle_duration     = std::chrono::seconds{2};
    
};