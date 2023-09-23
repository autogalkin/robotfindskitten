#pragma once

#include "engine/details/nonconstructors.h"
#include <functional>
#include <type_traits>
#pragma warning(push, 0)
#include "Windows.h"
#include "boost/container/static_vector.hpp"
#include <ranges>
#pragma warning(pop)
#include "engine/details/base_types.h"
#include "engine/details/gamelog.h"
#include <mutex>

namespace input {

using key_size = WPARAM;
// https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
enum class key_t : key_size {
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
static constexpr size_t state_capacity = 5;
using state_t = std::vector<key_t>;

[[nodiscard]] static bool has_key(const state_t& state, key_t key) {
    return std::ranges::any_of(state, [key](key_t k) { return k == key; });
}

class thread_input : public noncopyable, public nonmoveable {
  public:
    // 111111
    friend void swap(input::thread_input& input, state_t& other) {
        std::lock_guard<std::mutex> lock(input.mutex_);
        std::swap(input.key_state_, other);
    }
    void push(const key_t key) {
        std::lock_guard<std::mutex> lock(mutex_);
        key_state_.push_back(key);
    };
    thread_input() : key_state_(state_capacity) {}
    ~thread_input();

  private:
    mutable std::mutex mutex_;
    state_t key_state_;
};

} // namespace input
