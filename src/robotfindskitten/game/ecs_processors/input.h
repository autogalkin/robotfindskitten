#pragma once
// clang-format off
#include <iterator>
#include <optional>
#include <ranges>
#include <xutility>

#include "Windows.h"
#include <winuser.h>

#include "boost/signals2.hpp"
#include <entt/entt.hpp>

#include "engine/world.h"
// clang-format on

namespace input {
using key_size = WPARAM;
// https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
enum class key : key_size {
    w = 0x57,
    a = 0x41,
    s = 0x53,
    d = 0x44,
    l = 0x49,
    space = VK_SPACE,
    up = VK_UP,
    right = VK_RIGHT,
    left = VK_LEFT,
    down = VK_DOWN,
};
struct key_state {
    // NOLINTNEXTLINE(google-explicit-constructor)
    key_state(input::key k): key(k) {}
    input::key key;
    uint16_t press_count = 0;
};

using state_t = std::vector<key_state>;

[[maybe_unused]] [[nodiscard]] static std::optional<key_state>
has_key(const state_t& state, input::key key) {
    auto it = std::ranges::find_if(state,
                                   [key](key_state k) { return k.key == key; });
    return it == state.end() ? std::nullopt : std::make_optional(*it);
}

struct processor: ecs_proc_tag {
    struct down_signal {
        // for chain input in runtime
        boost::signals2::signal<void(entt::registry&, entt::entity,
                                     const state_t&, const timings::duration)>
            call;
    };
    std::vector<key_state> keys_ = {key::w, key::a, key::s, key::d, key::space};

    void execute(entt::registry& reg, timings::duration dt) {
        static constexpr WPARAM is_pressed = 0x8000;
        for(auto& i: keys_) {
            if(GetAsyncKeyState(static_cast<int>(i.key)) & is_pressed) {
                ++i.press_count;
            } else {
                i.press_count = 0;
            }
        }
        std::vector<key_state> state;
        state.reserve(keys_.size());
        std::copy_if(keys_.begin(), keys_.end(), std::back_inserter(state),
                     [](key_state k) { return k.press_count > 0; });
        if(state.empty()) {
            return;
        }
        for(const auto view = reg.view<const processor::down_signal>();
            const auto entity: view) {
            view.get<processor::down_signal>(entity).call(reg, entity, state,
                                                          dt);
        }
    }
};
} // namespace input
