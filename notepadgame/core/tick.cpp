#include "tick.h"

#include "notepader.h"

void tickable::connect_to_tick()
{ tick_connection = notepader::get().get_on_tick().connect([this]{tick();}); }
