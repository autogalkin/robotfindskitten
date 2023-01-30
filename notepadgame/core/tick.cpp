#include "tick.h"

#include "notepader.h"

tickable::~tickable()
{ tick_connection.disconnect();}

tickable& tickable::operator=(const tickable& other)
{
    if (this == &other)
        return *this;
    connect_to_tick();
    return *this;
}

tickable& tickable::operator=(tickable&& other) noexcept
{
    if (this == &other)
        return *this;
    other.tick_connection.disconnect();
    connect_to_tick();
    return *this;
}

void tickable::connect_to_tick()
{ tick_connection = notepader::get().get_on_tick().connect([this]{tick();}); }
