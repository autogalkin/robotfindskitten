#pragma once

#include "details/base_types.h"
#include "ecs_processor_base.h"
#include "engine.h"
#include "world.h"

class screen_change_handler final : public ecs_processor {
public:
  explicit screen_change_handler(world *w, engine *e) : ecs_processor(w) {
    size_changed_connection = e->get_on_resize().connect(
        [this](const uint32_t width, const uint32_t height) {
          on_resize(get_world()->get_registry(), width, height);
        });
    scroll_changed_connection_ =
        e->get_on_scroll_changed().connect([this](const position &new_scroll) {
          on_scroll(get_world()->get_registry(), new_scroll);
        });
  }

  void execute(entt::registry &reg, gametime::duration delta) override{
      // nothing to do
  };

  void on_resize(entt::registry &reg, const uint32_t width,
                 const uint32_t height) {
    for (const auto view = reg.view<screen_resize>();
         const auto entity : view) {
      auto &[screen_resize_do] = view.get<screen_resize>(entity);
      screen_resize_do(width, height);
    }
  }
  void on_scroll(entt::registry &reg, const position &new_scroll) {
    for (const auto view = reg.view<screen_scroll>();
         const auto entity : view) {
      auto &[screen_scroll_do] = view.get<screen_scroll>(entity);
      screen_scroll_do(new_scroll);
    }
  }

private:
  boost::signals2::scoped_connection scroll_changed_connection_;
  boost::signals2::scoped_connection size_changed_connection;
};
