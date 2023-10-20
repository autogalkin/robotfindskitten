
#ifndef _CPP_PROJECTS_ROBOTFINDSKITTEN_SRC_ROBOTFINDSKITTEN_GAME_ECS_PROCESSORS_INPUT_H
#define _CPP_PROJECTS_ROBOTFINDSKITTEN_SRC_ROBOTFINDSKITTEN_GAME_ECS_PROCESSORS_INPUT_H

#include <algorithm>
#include <iterator>
#include <optional>
#include <xutility>
#include <span>

#include <Windows.h>
#include <winuser.h>

#include <entt/entity/registry.hpp>
#include <entt/entity/handle.hpp>
#include <entt/signal/delegate.hpp>

#include "engine/notepad.h"

namespace input {

using key_size = WPARAM;
// Supported keys
// https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
enum class key : key_size {
    w = 0x57,
    a = 0x41,
    s = 0x53,
    d = 0x44,
    space = VK_SPACE,
};

/**
 * @brief A key and a count of key presses.
 */
struct key_state {
    input::key key{0};
    uint16_t press_count = 0;
};

using state_t = std::vector<key_state>;

/**
 * @brief Checks the key is pressed or not
 *
 * @param state The current keyboard state
 * @param key Which key
 * @return A state of the given key
 */
[[maybe_unused]] [[nodiscard]] static std::optional<key_state>
has_key(std::span<key_state> state, input::key key) {
    auto it = std::ranges::find_if(state,
                                   [key](key_state k) { return k.key == key; });
    return it == state.end() ? std::nullopt : std::make_optional(*it);
}

/**
 * @brief An ECS component with a reaction to key pressing
 */
struct key_down_task {
    using function_type =
        entt::delegate<void(entt::handle, std::span<key_state>)>;
    function_type exec;
};
/**
 * @brief If the \ref key_down_task is disabled inside the ecs execution graph
 */
struct task_disable_tag {};

/**
 * @briefA node among the many key_down_tasks within a single registry. For
 * additional input, we create a new entity and emplace a link to the player
 * entity.
 */
struct task_owner {
    entt::entity v = entt::null;
};
constexpr auto SUPPORTED_KEYS =
    std::to_array({key::w, key::a, key::s, key::d, key::space});

struct player_input {
    std::array<key_state, SUPPORTED_KEYS.size()> keys_;
    explicit player_input(entt::registry& reg) {
        [[maybe_unused]] auto initialize_group =
            reg.group<key_down_task>({}, entt::exclude<task_disable_tag>);
        for(size_t i = 0; i < keys_.size(); ++i) {
            keys_.at(i) = key_state{SUPPORTED_KEYS.at(i), 0};
        }
    }

    void operator()(entt::registry& reg) {
        if(!notepad::is_window_active()) {
            return;
        }
        static constexpr WPARAM IS_PRESSED = 0x8000;
        for(auto& i: keys_) {
            // I believe it has the right to exist... ¯\_(ツ)_/¯ it is fast...
            if(GetAsyncKeyState(static_cast<int>(i.key)) & IS_PRESSED) {
                ++i.press_count;
            } else {
                i.press_count = 0;
            }
        }
        state_t state;
        state.reserve(SUPPORTED_KEYS.size());
        std::copy_if(keys_.begin(), keys_.end(), std::back_inserter(state),
                     [](key_state k) { return k.press_count > 0; });
        if(state.empty()) {
            return;
        }
        for(const auto& [ent, task]:
            reg.group<key_down_task>({}, entt::exclude<task_disable_tag>)
                .each()) {
            task.exec(entt::handle{reg, ent}, state);
        }
    }
};

} // namespace input

#endif
