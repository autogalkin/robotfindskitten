#pragma once
#include <entt/entt.hpp>
#include "../core/base_types.h"


class movement final : public ecs_processor
{
public:
    explicit movement(world* w): ecs_processor(w){}
    void execute(entt::registry& reg) override;
};

