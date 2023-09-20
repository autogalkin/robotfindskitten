#include "tick.h"
#include "notepader.h"

tickable::tickable()
    : tick_connection(notepader::get().get_on_tick().connect(
          [this](const gametime::duration delta) { tick(delta); })) {}
