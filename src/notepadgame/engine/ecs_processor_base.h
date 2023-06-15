#pragma once
#include <entt/entt.hpp>
#include "tick.h"
#include "details/nonconstructors.h"
class world;




class ecs_processor: public noncopyable, public nonmoveable
{
public:
  explicit ecs_processor(world *w) : w_(w) {}
  virtual void execute(entt::registry &reg, gametime::duration delta) = 0;

  virtual ~ecs_processor() = default;
protected:
    [[nodiscard]] world* get_world() const {return w_;}
private:
    world* w_ = nullptr;
};