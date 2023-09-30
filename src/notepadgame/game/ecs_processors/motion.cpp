#include "game/ecs_processors/motion.h"
#include "engine/world.h"

void non_uniform_motion::execute(entt::registry& reg, timings::duration  /*dt*/) {
    for(const auto view =
            reg.view<velocity, translation, const non_uniform_movement_tag>();
        const auto entity: view) {
        auto& vel = view.get<velocity>(entity);
        auto& trans = view.get<translation>(entity);

        using namespace std::literals;
        //double alpha = std::chrono::duration<double>(dt / 1s).count();
        double alpha = 1./60.; // NOLINT(readability-magic-numbers)
        loc friction = -(vel * friction_factor);
        vel += friction * loc(alpha);
        // alpha = lag accum / dt
        // oldvel = vel;
        // vel += friction * dt/1s
        // (vel)* alpha + oldvel * (1-alpha)
        trans.pin() = vel * static_cast<double>(alpha);
    }
}

void uniform_motion::execute(entt::registry& reg, timings::duration /*dt*/) {
    for(const auto view =
            reg.view<translation, velocity, const uniform_movement_tag>();
        const auto entity: view) {
        auto& vel = view.get<velocity>(entity);
        auto& trans = view.get<translation>(entity);

        if(vel != loc(0)) {
            trans.pin() = vel;
        }

        vel = {0, 0};
    }
}
