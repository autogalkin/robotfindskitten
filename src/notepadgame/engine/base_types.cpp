#include "base_types.h"
#include "notepader.h"
#include "engine.h"


npi_t position_converter::to_notepad_index(const position& l)
{
    return  notepader::get().get_engine()->get_first_char_index_in_line( l.line() ) + l.index_in_line();
}

position position_converter::from_notepad_index(const npi_t i)
{
    auto& w = notepader::get().get_engine();
    return {w->get_line_index(i), i - w->get_first_char_index_in_line( w->get_line_index(i)) };
}
