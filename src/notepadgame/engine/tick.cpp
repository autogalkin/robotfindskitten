#include "tick.h"
#include "notepad.h"

tickable::tickable()
    : tick_connection(notepad_t::get().get_on_tick().connect(
          [this](const gametime::duration delta) { tick(delta); })) {}
