#pragma once

#include "Windows.h"
#include "boost/signals2.hpp"
#include "boost/container/static_vector.hpp"
#include <ranges>
#include <entt/entt.hpp>
#include <winuser.h>
#include <xutility>

#include "engine/details/base_types.h"
#include "engine/notepad.h"

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
using state_t =
    boost::container::static_vector<input::key_t, notepad::input_buffer_size>;
[[nodiscard]] static bool has_key(const state_t& state, input::key_t key) {
    return std::ranges::any_of(state,
                               [key](input::key_t k) { return k == key; });
}

struct key_state{
    input::key_t key;
    int8_t press;
};

struct processor{
    struct down_signal {
        // for chain input in runtime
        boost::signals2::signal<void(entt::registry&, entt::entity,
                                     const state_t&, const timings::duration)>
            call;
    };
    std::vector<key_state> buf_;

    void execute(entt::registry& reg, timings::duration dt){
        
        boost::container::static_vector<input::key_t,
                                        notepad::input_buffer_size>
            state;
        using namespace input;
        for(auto i : {key_t::w, key_t::a, key_t::s, key_t::d, key_t::space}){
            auto it = std::ranges::find_if(buf_, [i](key_state k){ return k.key == i;});
            if(GetAsyncKeyState(static_cast<WPARAM>(i)) & 0x8000){
                if(it != buf_.end()){
                    if(it->press==0 || !(it->press % 3) || it->press>30){
                        state.push_back(it->key);
                    } 
                    ++it->press;
                    
                } else {
                    buf_.push_back(key_state{i, 0});
                        state.push_back(i);
                }
            } else {
                if(it != buf_.end()){
                    it->press = 0;
                }
            }
        }
        if (state.empty())
            return;
        for (const auto view = reg.view<const processor::down_signal>();
             const auto entity : view) {
            view.get<processor::down_signal>(entity).call(reg, entity, state, dt);
        }
    }
};
} // namespace input
