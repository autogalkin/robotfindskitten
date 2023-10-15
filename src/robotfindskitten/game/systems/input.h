#pragma once
#ifndef _CPP_PROJECTS_ROBOTFINDSKITTEN_SRC_ROBOTFINDSKITTEN_GAME_ECS_PROCESSORS_INPUT_H
#define _CPP_PROJECTS_ROBOTFINDSKITTEN_SRC_ROBOTFINDSKITTEN_GAME_ECS_PROCESSORS_INPUT_H

// clang-format off
#include <iterator>
#include <optional>
#include <ranges>
#include <xutility>

#include <Windows.h>
#include <winuser.h>

#include <boost/signals2.hpp>
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
    // key_state(input::key k): key(k) {}
    input::key key;
    uint16_t press_count = 0;
};

using state_t = std::deque<key_state>;

[[maybe_unused]] [[nodiscard]] static std::optional<key_state>
has_key(const state_t& state, input::key key) {
    auto it = std::ranges::find_if(state,
                                   [key](key_state k) { return k.key == key; });
    return it == state.end() ? std::nullopt : std::make_optional(*it);
}

struct key_down_task {
    entt::delegate<void(entt::handle, state_t<key_state>, timings::duration)>
        exec;
};
struct task_disable_tag {};
struct next_task {
    entt::entity next = entt::null;
};
inline static constexpr auto SUPPORTED_KEYS =
    std::to_array<key_state>({key::w, key::a, key::s, key::d, key::space});

struct player_input: is_system {
    player_input(entt::registry& reg) {
        [[maybe_unused]] initialize_group =
            reg.group<key_down_task>(entt::exclude<task_disable_tag>{});
    }
    std::array<key_state, SUPPORTED_KEYS.size()> keys_{SUPPORTED_KEYS};
    void operator()(entt::registry& reg, timings::duration dt) {
        static constexpr WPARAM IS_PRESSED = 0x8000;
        for(auto& i: keys_) {
            // TODO(Igor)
            // I think i has right to live rather then pipe lock_free_queue
            // from the notepad thread, or no?...
            if(GetAsyncKeyState(static_cast<int>(i.key)) & IS_PRESSED) {
                ++i.press_count;
            } else {
                i.press_count = 0;
            }
        }
        state_t state;
        state.reserve(keys_.size());
        std::copy_if(keys_.begin(), keys_.end(), std::back_inserter(state),
                     [](key_state k) { return k.press_count > 0; });
        if(state.empty()) {
            return;
        }
        for(const auto&& [ent, task]:
            reg.group<key_down_task>(entt::exclude<task_disable_tag>{})) {
            task.exec(entt::handle{reg, entity}, state, dt);
        }
    }
};


} // namespace input

#endif
