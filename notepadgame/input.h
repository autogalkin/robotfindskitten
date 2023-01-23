#pragma once
#include <queue>

#include "boost/signals2.hpp"
#include <Windows.h>





class input
{
public:
    
    // https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
    enum class key{
        w = 0x57,
        a = 0x41,
        s = 0x53,
        d = 0x44,
    };

    enum class direction{
        up,
        down,
        undefined
    };

    struct key_signals
    {
        boost::signals2::signal<void (direction)> on_w;
        boost::signals2::signal<void (direction)> on_a;
        boost::signals2::signal<void (direction)> on_s;
        boost::signals2::signal<void (direction)> on_d;
    };
    
    input() : curr_input_code_(0), curr_direction_(direction::undefined){}
    ~input() = default;

    input(input& other) noexcept = delete;
    input(input&& other) noexcept = delete;
    input& operator=(const input& other) = delete;
    input& operator=(input&& other) noexcept // move assignment
    {
        std::swap(signals_, other.signals_);
        return *this;
    }
   
    [[nodiscard]] key_signals& get_signals() {return signals_; }
    void init();
    void receive_input();
    void set_input_message(const LPMSG msg);

private:
    key_signals signals_;
    // TODO queue?
   // std::queue<WPARAM> pressed_keys_;
    WPARAM curr_input_code_;
    direction curr_direction_;

};
