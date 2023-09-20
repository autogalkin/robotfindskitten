#include "motion.h"
#include "world.h"

void non_uniform_motion::execute(entt::registry& reg,
                                 gametime::duration delta) {

    for (const auto view =
             reg.view<location_buffer, velocity, non_uniform_movement_tag>();
         const auto entity : view) {

        auto& vel = view.get<velocity>(entity);
        auto& [current, translation] = view.get<location_buffer>(entity);

        constexpr float alpha = 1.f / 60.f;

        velocity friction = -(vel * friction_factor);
        vel += friction * alpha;

        translation.pin() = vel * static_cast<double>(alpha);
    }
}

void uniform_motion::execute(entt::registry& reg, gametime::duration delta) {

    for (const auto view =
             reg.view<location_buffer, velocity, const uniform_movement_tag>();
         const auto entity : view) {
        auto& vel = view.get<velocity>(entity);
        auto& [current, translation] = view.get<location_buffer>(entity);

        if (vel != velocity::null())
            translation.pin() = vel;

        vel = velocity::null();
    }
}
