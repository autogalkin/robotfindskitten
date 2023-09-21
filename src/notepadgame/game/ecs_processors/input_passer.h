#pragma once
#pragma warning(push, 0)
#include "Windows.h"
#include "boost/signals2.hpp"
#include <entt/entt.hpp>
#pragma warning(pop)

#include "details/base_types.h"
#include "ecs_processor_base.h"
#include "input.h"

class input_passer final : public ecs_processor {
  public:
    explicit input_passer(world* w, input_t* i) : ecs_processor(w), input_(i) {}

    struct down_signal {
        // for customize input in runtime
        boost::signals2::signal<void(entt::registry&, entt::entity,
                                     const input_t&)>
            call;
    };

    void execute(entt::registry& reg, gametime::duration) override {
        ;
        if (input_->is_empty())
            return;

        for (const auto view = reg.view<const input_passer::down_signal>();
             const auto entity : view) {

            view.get<input_passer::down_signal>(entity).call(reg, entity,
                                                             *input_);
        }
    }
    input_t* input_{nullptr};
};
