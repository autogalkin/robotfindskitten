#include "input.h"

#include "core/notepader.h"


void input::tick()
{
    send_input();
}

void input::send_input()
{
    if(!down_key_state_.empty())
    {
        on_down(down_key_state_);
        on_up(up_key_state_);
    }
    down_key_state_.clear();
    up_key_state_.clear();
}

void input::receive(const LPMSG msg)
{
    switch (msg->message)
    {
    case WM_KEYDOWN: 
    case WM_SYSKEYDOWN:{
            down_key_state_.push_back(static_cast<key>(msg->wParam));
            break;
        }
    case WM_KEYUP:   
    case WM_SYSKEYUP:{
            up_key_state_.push_back(static_cast<key>(msg->wParam));
            break;
        }
    default: return;
    }

}

