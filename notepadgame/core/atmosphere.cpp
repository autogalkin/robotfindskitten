#include "atmosphere.h"

#include "gamelog.h"
#include "world.h"
#include "notepader.h"


atmosphere::atmosphere(const spawner _, const std::chrono::seconds cycle, const color_range backgroung, const color_range foreground)
: actor(_, actor::whitespace), timeline_([this](){update();}, cycle, true), background(backgroung), foreground(foreground)
{
    timeline_.start();
    old_state_ =  notepader::get().get_world()->get_background_color();
}

atmosphere::~atmosphere()
{
    notepader::get().get_world()->set_background_color(old_state_);
}

void atmosphere::update() const
{
    
    // new_value = ( (old_value - old_min) / (old_max - old_min) ) * (new_max - new_min) + new_min
    const auto alpha = std::chrono::duration<double>(timeline_.get_last_time().time_since_epoch()).count();
    const auto start = std::chrono::duration<double>(timeline_.get_start_time().time_since_epoch()).count();
    const auto stop = std::chrono::duration<double>(timeline_.get_stop_time().time_since_epoch()).count();
    auto value = (alpha - start) / (stop - start);
    value = std::lerp(value, 1-value, std::max(static_cast<double>(timeline_.get_current_direction()), 0.));
    
    const COLORREF new_back_color = RGB(std::lerp(GetRValue(background.start), GetRValue(background.end), value)
                                 , std::lerp(GetGValue(background.start), GetGValue(background.end), value)
                                 , std::lerp(GetBValue(background.start), GetBValue(background.end), value));
    
    const COLORREF new_front_color = RGB(std::lerp(GetRValue(foreground.start), GetRValue(foreground.end), value)
                                     , std::lerp(GetGValue(foreground.start), GetGValue(foreground.end), value)
                                     , std::lerp(GetBValue(foreground.start), GetBValue(foreground.end), value));
    
    notepader::get().get_world()->force_set_background_color(new_back_color);
    notepader::get().get_world()->force_set_all_text_color(new_front_color);
    
}
