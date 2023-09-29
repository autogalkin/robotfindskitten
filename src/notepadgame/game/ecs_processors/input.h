#pragma once

#include "Windows.h"
#include "boost/signals2.hpp"
#include "boost/container/static_vector.hpp"
#include <iterator>
#include <optional>
#include <ranges>
#include <entt/entt.hpp>
#include <winuser.h>
#include <xutility>

#include "engine/details/base_types.h"
#include "engine/notepad.h"

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
    key_state(input::key k) : key(k) {}
    input::key key;
    int8_t press_count;
};
using state_t =
    boost::container::static_vector<key_state,8>;
[[nodiscard]] static std::optional<key_state> has_key(const state_t& state, input::key key) {
    auto it = std::ranges::find_if(state,
                               [key](key_state k) { return k.key == key; });
    return it == state.end() ? std::nullopt : std::make_optional(*it);

}


struct processor {
    struct down_signal {
        // for chain input in runtime
        boost::signals2::signal<void(entt::registry&, entt::entity,
                                     const state_t&, const timings::duration)>
            call;
    };
    std::vector<key_state> keys_ = {key::w, key::a, key::s, key::d,
                                    key::space};

    void execute(entt::registry& reg, timings::duration dt) {

        using namespace input;
        for (auto& i : keys_) {
            if (GetAsyncKeyState(static_cast<WPARAM>(i.key)) & 0x8000) {
                ++i.press_count;
            } else {
                i.press_count = 0;
            }
        }
        boost::container::static_vector<key_state,
                                        8>
            state;
        std::copy_if(keys_.begin(), keys_.end(), std::back_inserter(state),
                     [](key_state k) { return k.press_count > 0; });
        if (state.empty())
            return;
        for (const auto view = reg.view<const processor::down_signal>();
             const auto entity : view) {
            view.get<processor::down_signal>(entity).call(reg, entity, state,
                                                          dt);
        }
    }
};
} // namespace input
