#include "base_types.h"
#include "engine.h"
#include "notepad.h"

npi_t position_converter::to_notepad_index(const position_t& l) {
    return notepad_t::get().get_engine().get_first_char_index_in_line(
               l.line()) +
           l.index_in_line();
}

position_t position_converter::from_notepad_index(const npi_t i) {
    auto& w = notepad_t::get().get_engine();
    return {w.get_line_index(i),
            i - w.get_first_char_index_in_line(w.get_line_index(i))};
}
