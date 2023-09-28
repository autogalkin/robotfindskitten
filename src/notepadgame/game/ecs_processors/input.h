#pragma once

#include "Windows.h"
#include "boost/signals2.hpp"
#include "boost/container/static_vector.hpp"
#include <ranges>
#include <entt/entt.hpp>

#include "engine/details/base_types.h"
#include "engine/ecs_processor_base.h"
#include "engine/notepad.h"

namespace input {
using state_t =
    boost::container::static_vector<input::key_t, notepad::input_buffer_size>;
[[nodiscard]] static bool has_key(const state_t& state, input::key_t key) {
    return std::ranges::any_of(state,
                               [key](input::key_t k) { return k == key; });
}
class processor{
  public:
    struct down_signal {
        // for chain input in runtime
        boost::signals2::signal<void(entt::registry&, entt::entity,
                                     const state_t&)>
            call;
    };

    void execute(entt::registry& reg, timings::duration) override {
        // swap data from other thread
        boost::container::static_vector<input::key_t,
                                        notepad::input_buffer_size>
            state;
        notepad::consume_input(
            [&state](input::key_t k) { state.push_back(k); });
        if (state.empty())
            return;
        for (const auto view = reg.view<const processor::down_signal>();
             const auto entity : view) {
            view.get<processor::down_signal>(entity).call(reg, entity, state);
        }
    }
    // input::state_t state_;
};
} // namespace input
