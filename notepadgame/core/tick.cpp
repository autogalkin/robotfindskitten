#include "tick.h"

#include "notepader.h"


tickable::tickable():
        tick_connection(notepader::get().get_on_tick().connect(
            [this]
            {
                tick();
            }
            ))
{
}

void timer::start() noexcept 
{
    assert(!started_ || !stopped_ && "timer::start: the timer has already beed started or finished");
    started_ = true;
    last_time_ = ticker::clock::now();
    start_time_ = last_time_;
    stop_time_ =  start_time_ + rate_;
    
}

// TODO last time можно оптимизировать убрать
void timer::tick()
{
    if(is_started() && !is_finished())
    {
        if(update_last_time(); get_last_time() >= get_stop_time())
        {
            call_do();
            
            if(is_loopping()) restart();
            else
            {
                stop();
            }
        }
    }
}


void timeline::tick()
{
    if(is_started() && !is_finished())
    {
        call_do();
        
        if(update_last_time(); get_last_time() >= get_stop_time())
        {
            if(is_loopping())
            {
                invert_direction();
                restart();
            } 
            else
            {
                stop();
            }
        }
    }
    
}
