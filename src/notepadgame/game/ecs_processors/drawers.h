﻿#pragma once


#include "details/base_types.h"
#include "ecs_processor_base.h"
#include "world.h"




class rotate_animator  final : public ecs_processor
{
public:
    explicit rotate_animator (world* w)
        : ecs_processor(w){
    }

    void execute(entt::registry& reg, gametime::duration delta) override
    {
        for(const auto view = reg.view<location_buffer, shape::sprite_animation, shape::on_change_direction, shape::render_direction>();
        const auto entity: view)
        {
            auto& [current, translation] = view.get<location_buffer>(entity);

            if(auto& [dir] = view.get<shape::render_direction>(entity)
                ;  static_cast<int>(translation.get().index_in_line()) != 0
                && static_cast<int>(std::copysign(1, translation.get().index_in_line())) != static_cast<int>(dir))
            {
                dir = direction_converter::invert(dir);
                view.get<shape::on_change_direction>(entity).call(dir, view.get<shape::sprite_animation>(entity));
                
                translation.pin().index_in_line() = 0.;
            }
        }
    }
};


class redrawer final : public ecs_processor
{
private:
    struct previous_sprite{ int8_t index = -1;};
public:
    explicit redrawer(world* w): ecs_processor(w){
        
       w->get_registry().on_construct<visible_in_game>().connect<&redrawer::upd_visible>();
    }

    void execute(entt::registry& reg, gametime::duration delta) override
    {
        
        for(const auto view = reg.view<const location_buffer, const shape::sprite_animation, previous_sprite>()
            ; const auto entity: view)
        {
            const bool is_dead =  reg.all_of<life::begin_die>(entity);
            if(const auto& [current, translation] = view.get<location_buffer>(entity); translation.is_changed() || is_dead){
                
                const auto& [sprts, rendering_i] = view.get<shape::sprite_animation>(entity);
                get_world()->erase(position_converter::from_location(view.get<location_buffer>(entity).current)
                             , sprts[view.get<previous_sprite>(entity).index]);
                if(is_dead) reg.remove<visible_in_game>(entity);
                
            }
        }
        
        for(const auto view = reg.view<location_buffer, const shape::sprite_animation, const visible_in_game>();
        const auto entity: view)
        {
            auto& [current, translation] = view.get<location_buffer>(entity);
            
            if(translation.is_changed()){
                
                auto& sp = view.get<shape::sprite_animation>(entity);
                
                current += translation.get();
                get_world()->draw (position_converter::from_location(current), sp.current_sprite());
                
                translation = dirty_flag<location>{location::null(), false};

                reg.emplace_or_replace<previous_sprite>(entity, static_cast<int8_t>(sp.rendering_i));
            }
        }
    }
private:
    static void upd_visible(entt::registry& reg, const entt::entity e){
        if(const auto ptr = reg.try_get<location_buffer>(e)) ptr->translation.mark_dirty();
    }
};

