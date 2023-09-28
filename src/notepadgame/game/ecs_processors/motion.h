#pragma once
#include "engine/details/base_types.h"
#include "engine/notepad.h"
#include "engine/world.h"
#include "engine/time.h"


struct uniform_movement_tag {};
struct non_uniform_movement_tag {};
struct velocity : vec_t<float> {
    velocity() = default;
    velocity(const vec_t<float>& l) : vec_t<float>(l) {}
    template <typename T>
    velocity(const Eigen::EigenBase<T>& other) : vec_t<float>{other} {}
    template <typename T>
    velocity(const Eigen::EigenBase<T>&& other) : vec_t<float>{other} {}
    velocity(const float line, const float index_in_line)
        : vec_t<float>{line, index_in_line} {}
};



struct non_uniform_motion{
    inline static const velocity friction_factor{0.7f, 0.7f};

    void execute(entt::registry& reg, timings::duration delta);
};

struct uniform_motion  {
    void execute(entt::registry& reg, timings::duration delta);
};
