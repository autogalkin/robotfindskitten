#include "motion.h"


#include "../core/world.h"

void non_uniform_motion::execute(entt::registry& reg, gametime::duration delta)
{
    
    for(const auto view = reg.view<location_buffer, const shape, velocity, non_uniform_movement_tag>();
        const auto entity: view)
    {
            
        auto& vel = view.get<velocity>(entity);
        const auto& sh = view.get<shape>(entity);
        auto& [current, translation] = view.get<location_buffer>(entity);

        constexpr float alpha = 1.f/60.f;
        
        velocity friction = - (vel * friction_factor);
        vel += friction * alpha;
        
        translation = vel * alpha;
        
        
    }
}

void uniform_motion::execute(entt::registry& reg, gametime::duration delta)
{
    
    for(const auto view = reg.view<location_buffer, const shape, velocity, const uniform_movement_tag>();
        const auto entity: view)
    {
        auto& vel = view.get<velocity>(entity);
        const auto& sh = view.get<shape>(entity);
        auto& [current, translation] = view.get<location_buffer>(entity);
        
        translation = vel;
        vel = velocity::null();
    }
        
}
