#pragma once
#include "details/base_types.h"
#include "details/gamelog.h"
#include "ecs_processor_base.h"
#include "notepad.h"
#include "world.h"

class render_commands final : public ecs_processor {
  thread_commands* commands_;
  thread_commands::queue_t local_queue_;
  public:
    explicit render_commands(world* w,  thread_commands* q) : ecs_processor(w), commands_(q), local_queue_(32) {}
    void execute(entt::registry& reg, gametime::duration delta) override{
        for (const auto view = reg.view<render_command>();
             const auto entity : view) {
            local_queue_.push_back(std::exchange(view.get<render_command>(entity).command, std::function<void(scintilla*)>{}));
        }
        swap(local_queue_, *commands_);
        local_queue_.clear();
    }
};


