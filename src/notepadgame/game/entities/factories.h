#pragma once

#include "df/dirtyflag.h"

#include "easing.h"
#include "details/base_types.h"
#include "notepader.h"
#include "ecs_processors/collision.h"
#include "input.h"

struct coin;
struct character;

struct actor
{
    static void invert_shape_direction(direction d, shape::sprite_animation& sp){
        sp.rendering_i = static_cast<uint8_t>(std::max(0, -1*static_cast<int>(d)));
    }
    static void make_base_renderable(entt::registry& reg, const entt::entity e, location start, shape::sprite sprite)
    {
        reg.emplace<visible_in_game>(e);
        reg.emplace<location_buffer>(e, std::move(start), df::dirtyflag<location>{{}, df::state::dirty});
        reg.emplace< shape::sprite_animation>(e,  std::vector<shape::sprite>{{std::move(sprite)}}, static_cast<uint8_t>(0));
    }
};
struct projectile final
{
    static collision::responce on_collide(entt::registry& r, const entt::entity self, const  entt::entity other)
    {
        r.emplace_or_replace<life::begin_die>(self);
        return collision::responce::block;
    }
    static void make(entt::registry& reg, const entt::entity e
        , location start
        , velocity dir
        ,  const std::chrono::milliseconds life_time=std::chrono::seconds{1})
    {
        reg.emplace<projectile>(e);
        reg.emplace< shape::sprite_animation>(e,  std::vector<shape::sprite>{{shape::sprite::from_data{U"-", 1, 1}}}, static_cast<uint8_t>(0));
        reg.emplace<visible_in_game>(e);
        
        reg.emplace<location_buffer>(e, std::move(start), df::dirtyflag<location>{{}, df::state::dirty});
        reg.emplace<non_uniform_movement_tag>(e);
        reg.emplace<velocity>(e, dir);
        
        reg.emplace<collision::agent>(e);
        
        reg.emplace<collision::on_collide>(e, &projectile::on_collide);
        
        reg.emplace<life::lifetime>(e, life_time);

        
        reg.emplace< timeline::what_do>(e, [duration=std::chrono::duration<double>(life_time).count(), call = true] (entt::registry& reg_, const entt::entity e_, direction)
        mutable {
            auto& i = reg_.get<location_buffer>(e_).translation.pin().line();
            i = easing::easeinrange(duration - std::chrono::duration<double>(reg_.get<life::lifetime>(e_).duration).count(), {0., 0.1}, duration, &easing::easeInExpo);
            // TODO почему i = 0.1 Но прибавляется строка
            if(std::lround(i*10) != 0 && call){
                reg_.get<shape::sprite_animation>(e_).current_sprite().data(0, 0) = '_';
                call = false;
            }
        });
        
        reg.emplace< timeline::eval_direction> (e, direction::forward);
        reg.emplace<life::death_last_will>(e, [](entt::registry& reg_, const entt::entity ent)
        {
            const auto& [current_proj_location, translation] = reg_.get<location_buffer>(ent);
            
            const auto new_e = reg_.create();
            reg_.emplace<location_buffer>(new_e, current_proj_location, df::dirtyflag<location>{location{translation.get()}, df::state::dirty});
            reg_.emplace<shape::sprite_animation>( new_e, std::vector<shape::sprite>{ {shape::sprite::from_data{U"*", 1, 1} }}, static_cast<uint8_t>(0));
            reg_.emplace<visible_in_game>(new_e);
            reg_.emplace<life::lifetime>(new_e, std::chrono::seconds{1});
            
            reg_.emplace<life::death_last_will>(new_e, [](entt::registry& r_, const entt::entity ent_)
            {
                auto& lb = r_.get<location_buffer>(ent_);
                
                auto create_fragment = [&r_, &lb](location offset, velocity dir_)
                {
                    const entt::entity proj = r_.create();
                    r_.emplace< shape::sprite_animation>(proj,  std::vector<shape::sprite>{{shape::sprite::from_data{U".", 1, 1}}}, static_cast<uint8_t>(0));
                    r_.emplace<location_buffer>(proj, location{lb.current.line()+offset.line(), lb.current.index_in_line()+offset.index_in_line()}, df::dirtyflag<location>{{}, df::state::dirty});
                    r_.emplace<visible_in_game>(proj);
                    r_.emplace<velocity>(proj, dir_.line(), dir_.index_in_line());
                    r_.emplace<life::lifetime>(proj, std::chrono::seconds{1});
                    r_.emplace<non_uniform_movement_tag>(proj);
                    r_.emplace<collision::agent>(proj);
                    r_.emplace<collision::on_collide>(proj, &projectile::on_collide);
                };
                
                create_fragment({0, 1}, {0, 2});
                create_fragment({0, -1}, {0, -2});
                create_fragment({0, 0}, {1, 0});
                create_fragment({-1, 0}, {-1, 0});
                
            });
        });
        
    }
};
struct gun
{

    static collision::responce on_collide(entt::registry& reg, const entt::entity self, const entt::entity collider)
    {
        if(reg.all_of<character>(collider))
        {
            reg.emplace_or_replace<life::begin_die>(self);
            return collision::responce::ignore;
        }
        return collision::responce::block;
    }
    
    static void make(entt::registry& reg, const entt::entity e, location loc)
    {
        reg.emplace<gun>(e);
        actor::make_base_renderable(reg, e, std::move(loc), {shape::sprite::from_data{U"¬", 1, 1}});
        reg.emplace<collision::agent>(e);
        reg.emplace<collision::on_collide>(e, &gun::on_collide);
        
    }
    
    static void fire(entt::registry& reg, const entt::entity character)
    {
        const auto& [loc, _] = reg.get<location_buffer>(character);
        auto& sh = reg.get<shape::sprite_animation>(character);
        auto [dir] = reg.get<shape::render_direction>(character);
                    
        const auto proj = reg.create();
        const location spawn_translation =  dir == direction::forward ? location{0, static_cast<double>(sh.current_sprite().bound_box().size.index_in_line() )} : location{0, -1};
        projectile::make(reg, proj,loc + spawn_translation,  velocity{0, 15 * static_cast<float>(dir)}, std::chrono::seconds{4});
                    
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
        reg.emplace<life::lifetime>(e, duration);
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
        reg.emplace<life::lifetime>(e, duration);
        reg.emplace<life::death_last_will>(e, std::move(what_do));
    }
};

struct character final
{
    static collision::responce on_collide(entt::registry& reg, const entt::entity self, const entt::entity collider)
    {
        // TODO collision functions without cast?
        if(reg.all_of<gun>(collider))
        {
            auto& [signal] = reg.get<input_passer::down_signal>(self);
            signal.connect([](entt::registry& reg_, const entt::entity chrcter, const input::key_state_type& state)
            {
                if(const auto it = std::ranges::find(state, input::key::space); it != state.end())
                {
                    gun::fire(reg_, chrcter);
                }
            });
            return collision::responce::ignore;
        }
        /*
        if(reg.all_of<coin>(collider))
        {
            return collision::responce::ignore;
        }
        */
        return collision::responce::block;
    }
    
    static void make(entt::registry& reg, const entt::entity e, location l)
    {
        reg.emplace<character>(e);
        reg.emplace<shape::on_change_direction>(e, &actor::invert_shape_direction);
        reg.emplace<shape::render_direction>(e, direction::forward);
        reg.emplace<visible_in_game>(e);
        reg.emplace<location_buffer>(e, std::move(l), df::dirtyflag<location>{{}, df::state::dirty});
        reg.emplace<uniform_movement_tag>(e);
        reg.emplace<velocity>(e);
        reg.emplace<collision::agent>(e);
        reg.emplace<collision::on_collide>(e, &character::on_collide);
    }
    
    static void add_top_down_camera(entt::registry& reg, const entt::entity e)
    {
        reg.emplace<timeline::eval_direction>(e);
        reg.emplace<timeline::what_do>(e, [](entt::registry& reg_, const entt::entity owner, direction)
        {
            const auto& [location, _] = reg_.get<location_buffer>(owner);
            auto pos = position_converter::from_location(location);
            
            
            const auto& scroll = notepader::get().get_engine()->get_scroll();
            const npi_t width =  notepader::get().get_engine()->get_window_widht() / notepader::get().get_engine()->get_char_width();
            const npi_t height = notepader::get().get_engine()->get_lines_on_screen();

            int c_move_pos = 0;
            if(pos.index_in_line() > width + scroll.index_in_line() || pos.index_in_line() < scroll.index_in_line())
            {
                c_move_pos = static_cast<int> ( pos.index_in_line() - scroll.index_in_line() -  width / 2); 
            }
            int l_move_pos = 0;
            if(pos.line() > height + scroll.line() || pos.line() < scroll.line())
            {
                l_move_pos = static_cast<int> ( pos.line() - scroll.line() -  height / 2);
            }
            
            if(l_move_pos !=0 || c_move_pos != 0)
            {
                reg_.remove<timeline::what_do>(owner);
                reg_.remove<timeline::eval_direction>(owner);
                
                const auto scroll_timeline = reg_.create();
                
                reg_.emplace<timeline::eval_direction>(scroll_timeline);
                
                // TODO refactor this
                reg_.emplace<timeline::what_do>(scroll_timeline, [
                    lifetime = 0,
                    t = 0, i = 0, j = 0
                    , c_move_pos,  c_sign = static_cast<int>(std::copysign(1, c_move_pos))
                    , l_move_pos,  l_sign = static_cast<int>(std::copysign(1, l_move_pos))]
                    (entt::registry& registry, const entt::entity scr_timeline, direction) mutable
                {
                    if(lifetime > std::max(std::abs(c_move_pos), std::abs(l_move_pos))){
                        registry.emplace<life::begin_die>(scr_timeline);
                    }
                    if(t > 3)
                    {
                        
                        t = 0;
                        int l = 1 * l_sign;
                        int c = 1 * c_sign;
                        
                        if(j == c_move_pos)
                        {
                            c = 0;
                        }
                        else
                        {
                            j += 1 * c_sign;
                        }
                        if(i == l_move_pos){
                            l = 0;
                        }
                        else{
                            i += 1 * l_sign; 
                        }
                        notepader::get().get_engine()->scroll(c, l);
                        ++ lifetime;
                    }
                    ++t;

                });
                
                reg_.emplace<life::death_last_will>(scroll_timeline, [owner](entt::registry& r_, const entt::entity)
                {
                    add_top_down_camera(r_, owner);
                });
            }
            
            
        });
    }
    
    template< input::key UP   =input::key::w
            , input::key LEFT =input::key::a
            , input::key DOWN =input::key::s
            , input::key RIGHT=input::key::d
    >
    static void process_movement_input(entt::registry& reg, const entt::entity e, const input::key_state_type& state)
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
       
        const auto cycle_timeline = reg.create();

        
        timeline::make(reg, cycle_timeline,  &atmosphere::update_cycle, cycle_duration, reg.get<timeline::eval_direction>(timer).value);
        
        reg.emplace<color_range>(cycle_timeline);
        reg.emplace<life::death_last_will>(cycle_timeline, [](entt::registry& reg_, const entt::entity cycle_timeline_){
            
            const auto again_timer = reg_.create();
            timer::make(reg_, again_timer, &atmosphere::run_cycle, time_between_cycle);
            reg_.emplace<timeline::eval_direction>(again_timer,  direction_converter::invert(reg_.get<timeline::eval_direction>(cycle_timeline_).value));
            
        });
        
    }
    static void update_cycle(entt::registry& reg, const entt::entity e, const direction d)
    {
        const auto& [start, end] = reg.get<color_range>(e);
        const auto& [current_lifetime] = reg.get<life::lifetime>(e);
        
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

struct monster
{
    static collision::responce on_collide(entt::registry& reg, const entt::entity self, const entt::entity collider)
    {
        if(reg.all_of<projectile>(collider))
        {
            reg.emplace_or_replace<life::begin_die>(self);
        }
        return collision::responce::block;
    }
    static void make(entt::registry& reg, const entt::entity e, location loc)
    {
        actor::make_base_renderable(reg, e, std::move(loc), {shape::sprite::from_data{U"👾", 1, 1}});
        
        reg.emplace<collision::agent>(e);
        reg.emplace<collision::on_collide>(e, &monster::on_collide);
        
        reg.emplace<velocity>(e);
        reg.emplace<uniform_movement_tag>(e);

        
        reg.emplace<timeline::eval_direction>(e, direction::forward);
        reg.emplace<timeline::what_do>(e, [i = 0, j = 0](entt::registry& reg_, const entt::entity e_, direction d) mutable
        {
            ++i;
            
            if(i > 30)
            {
                i = 0;
                ++ j;
                reg_.get<velocity>(e_).index_in_line() += static_cast<float>(d);
            }
            if(j > 15)
            {
                j = 0;
                reg_.get<timeline::eval_direction>(e_).value = direction_converter::invert(d);
            }
        });

        reg.emplace<life::death_last_will>(e, [](entt::registry& reg_, const entt::entity self)
        {
            const auto dead = reg_.create();
            auto& [old_loc, _] = reg_.get<location_buffer>(self);

            actor::make_base_renderable(reg_, dead, location{old_loc.line(),old_loc.index_in_line() - 1}, {shape::sprite::from_data{U"___", 1, 3}});
            reg_.emplace<life::lifetime>(dead, std::chrono::seconds{3});
            reg_.emplace<timeline::eval_direction>(dead);
            reg_.emplace<timeline::what_do>(dead, [](entt::registry& r_, const entt::entity e_, direction d){
                r_.get<location_buffer>(e_).translation.mark();
            });
            
            reg_.emplace<life::death_last_will>(dead, [](entt::registry& r_, const entt::entity s)
            {
                const auto timer = r_.create();
                
                r_.emplace<life::lifetime>(timer, std::chrono::seconds{10});
                r_.emplace<life::death_last_will>(timer, [spawn_loc = r_.get<location_buffer>(s).current](entt::registry& r1_, const entt::entity self_)
                {
                    const auto new_monster = r1_.create();
                    monster::make(r1_, new_monster, spawn_loc);
                });
            });
        });
    }
    
};
/*
struct coin
{
    
    static void make(entt::registry& reg, const entt::entity e, location loc)
    {
        reg.emplace<coin>(e);
        reg.emplace<visible_in_game>(e);
        reg.emplace<location_buffer>(e, std::move(loc), dirty_flag<location>{});
        reg.emplace< shape::sprite_animation>(e,  std::vector<shape::sprite>{
            {
                {shape::sprite::from_data{U"O", 1, 1}}
                ,{shape::sprite::from_data{U"0", 1, 1}}
            }}, static_cast<uint8_t>(0));
        reg.emplace<collision::agent>(e);
        reg.emplace<collision::on_collide>(e, [](entt::registry& r, const entt::entity self, const  entt::entity collider)
        {
            if(r.all_of<character>(collider))
            {
                r.emplace<life::begin_die>(self);
            }
            return collision::responce::ignore;
        });

        reg.emplace< timeline::eval_direction> (e, direction::forward);
        
        reg.emplace< timeline::what_do> (e, [t = std::rand() % 20 - 0 + 1, i = 1](entt::registry& reg_ , const entt::entity self_, direction) mutable 
        {
            if(t > 50)
            {
                t = 0;
                reg_.get<shape::sprite_animation>(self_).rendering_i = i;
                reg_.get<location_buffer>(self_).translation.mark_dirty();
                i = 1 - i;
            }
            ++t;
        });
    }
};
*/