#include "base_types.h"

#include "notepader.h"
#include "engine.h"


npi_t location_converter::to_notepad_index(const location& l)
{
    return  notepader::get().get_engine()->get_first_char_index_in_line( l.line() ) + l.index_in_line();
}

location location_converter::from_notepad_index(const npi_t i)
{
    auto& w = notepader::get().get_engine();
    return {w->get_line_index(i), i - w->get_first_char_index_in_line( w->get_line_index(i)) };
}
