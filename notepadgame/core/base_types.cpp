#include "base_types.h"

#include "notepader.h"
#include "world.h"
#include  "../world/level.h"
int64_t translation::to_index() noexcept
{
    return  notepader::get().get_world()->get_first_char_index_in_line( line() ) + index_in_line();
}

translation translation::from_index(const int64_t index) noexcept
{
    auto& w = notepader::get().get_world();
    return {w->get_line_index(index), index - w->get_first_char_index_in_line( w->get_line_index(index)) };
}
