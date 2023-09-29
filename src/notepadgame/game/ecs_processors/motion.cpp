#include "game/ecs_processors/motion.h"
#include "engine/world.h"

void non_uniform_motion::execute(entt::registry& reg, timings::duration delta) {

    for (const auto view =
             reg.view<velocity, translation, non_uniform_movement_tag>();
         const auto entity : view) {

        auto& vel = view.get<velocity>(entity);
        auto& trans = view.get<translation>(entity);

        constexpr float alpha = 1.f / 60.f;

        loc friction = -(vel * friction_factor);
        vel += friction * loc(alpha);

        trans.pin() = vel * static_cast<double>(alpha);
    }
}

void uniform_motion::execute(entt::registry& reg, timings::duration delta) {

    for (const auto view =
             reg.view<translation, velocity, const uniform_movement_tag>();
         const auto entity : view) {
        auto& vel = view.get<velocity>(entity);
        auto& trans = view.get<translation>(entity);

        if (vel != loc(0)) {
            trans.pin() = vel;
        }

        vel = {0, 0};
    }
}
