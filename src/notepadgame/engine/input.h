#pragma once

#pragma warning(push, 0)
#include "Windows.h"
#pragma warning(pop)
#include "details/base_types.h"


// Very simple Set of keys without ordering
class input_state_t {
  friend class notepad_t;
  friend bool hook_GetMessageW(const HMODULE module);
  using state_t = WPARAM;
  public:
    // https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
    enum class key_t : state_t {
        w = 0x57,
        a = 0x41,
        s = 0x53,
        d = 0x44,
        l = 0x49,
        space = VK_SPACE,
        up = VK_UP,
        right = VK_RIGHT,
        left = VK_LEFT,
        down = VK_DOWN

    };
    ~input_state_t();
    [[nodiscard]] state_t state() const {
        return key_state_;
    }
    [[nodiscard]] bool has_key(key_t key) const {
        return static_cast<int>(key_state_) & static_cast<int>(key);
    }
    [[nodiscard]] bool is_empty() const {
        return static_cast<int>(key_state_) == 0;
    }
private: 
    void push(const key_t key){
        key_state_ = static_cast<WPARAM>(key_state_ | static_cast<WPARAM>(key));
    };
    void clear() { key_state_ = 0; }
    state_t key_state_;
};
