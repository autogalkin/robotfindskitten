#pragma once
#include "engine/details/gamelog.h"

#include "Windows.h"
#include "boost/signals2.hpp"
#include <entt/entt.hpp>
#pragma warning(pop)

#include "engine/details/base_types.h"
#include "engine/ecs_processor_base.h"
#include "engine/input.h"

class input_passer final : public ecs_processor {
  public:
    explicit input_passer(world* w, input::thread_input* i)
        : ecs_processor(w), input_(i), state_(input::state_capacity) {}

    struct down_signal {
        // for customize input in runtime
        boost::signals2::signal<void(entt::registry&, entt::entity,
                                     const input::state_t&)>
            call;
    };

    void execute(entt::registry& reg, time2::duration) override {
        // swap data from other thread
        swap(*input_, state_);
        if (state_.empty())
            return;
        for (const auto view = reg.view<const input_passer::down_signal>();
             const auto entity : view) {

            view.get<input_passer::down_signal>(entity).call(reg, entity,
                                                             state_);
        }
        state_.clear();
    }
    input::thread_input* input_{nullptr};
    input::state_t state_;
};
