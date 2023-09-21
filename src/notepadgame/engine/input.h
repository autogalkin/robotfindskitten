#pragma once

#include <functional>
#pragma warning(push, 0)
#include "Windows.h"
#include "boost/container/static_vector.hpp"
#include <ranges>
#pragma warning(pop)
#include "details/base_types.h"
#include "details/gamelog.h"

// Very simple Set of keys without ordering
class input_t {
  friend class notepad_t;
  friend bool hook_GetMessageW(const HMODULE module);
  public:
    using key_size = WPARAM;
    // https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
    enum class key_t : key_size {
        w = 0x57,
        a = 0x41,
        s = 0x53,
        d = 0x44,
        l = 0x49,
        space = VK_SPACE,
        up    = VK_UP,
        right = VK_RIGHT,
        left  = VK_LEFT,
        down  = VK_DOWN,
    };
    using state_t = boost::container::static_vector<key_t, 4>;
    [[nodiscard]] const state_t& state() const noexcept {
        return key_state_;
    }
    [[nodiscard]] bool has_key(key_t key) const noexcept {
        return std::ranges::any_of(key_state_, [key](key_t k) {
                return k == key;});
    }
    [[nodiscard]] bool is_empty() const noexcept {
        return key_state_.empty();
    }
    ~input_t();
private: 
    state_t key_state_;
    void push(const key_t key){
        if(key_state_.size() <= key_state_.max_size())
            key_state_.push_back(key);
    };
    void clear() { key_state_.clear(); }
};
