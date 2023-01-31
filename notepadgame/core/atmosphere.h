#pragma once
#include "tick.h"
#include "Windows.h"
#include "../world/actor.h"


class atmosphere final : public actor
{
public:
    
    struct color_range
    {
        COLORREF start{RGB(0, 0, 0)};
        COLORREF end{RGB(255, 255, 255)};
    };
    
    explicit atmosphere(spawner, const std::chrono::seconds cycle, const color_range backgroung, const color_range foreground);
    explicit atmosphere(const actor& other)        = delete;
    explicit atmosphere(actor&& other)             = delete;
    atmosphere& operator=(const atmosphere& other) = delete;
    atmosphere& operator=(atmosphere&& other)      = delete;
    ~atmosphere() override;
private:
    void update() const;
    timer timer_;
    timeline timeline_;
    color_range background;
    color_range foreground;
    COLORREF old_state_;
};
