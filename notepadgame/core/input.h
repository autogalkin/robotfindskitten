#pragma once
#include <queue>

#include "boost/signals2.hpp"
#include <Windows.h>

#include "tick.h"



class input final : tickable
{
public:
    
    // https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
    enum class key : WPARAM {
          w = 0x57
        , a = 0x41
        , s = 0x53
        , d = 0x44
        , space = VK_SPACE
    };

    explicit input() = default;
    ~input() override;

    input(input& other) noexcept = delete;
    input(input&& other) noexcept = delete;
    input& operator=(const input& other) = delete;
    input& operator=(input&& other) noexcept = delete;

    virtual void tick() override;
    void send_input();
    void receive(const LPMSG msg);
    
    using key_state_type = std::vector<input::key>;
    using signal_type = boost::signals2::signal<void (const key_state_type& key_state)>;
    
    [[nodiscard]] input::signal_type& get_down_signal() {return on_down; }

private:
    
    signal_type on_down;
    signal_type on_up;
    key_state_type down_key_state_;
    key_state_type up_key_state_;

};
