#include "actor.h"
#include "../core/notepader.h"


int64_t translation::to_index() noexcept
{
    return  notepader::get().get_world()->get_first_char_index_in_line( line() ) + index_in_line();
}

translation translation::from_index(const int64_t index) noexcept
{
    auto& w = notepader::get().get_world();
    return {w->get_line_index(index), index - w->get_first_char_index_in_line( w->get_line_index(index)) };
}

mesh_actor& mesh_actor::operator=(const mesh_actor& other)
{
    if (this == &other)
        return *this;
    mesh_ = other.mesh_;
    return *this;
}

mesh_actor& mesh_actor::operator=(mesh_actor&& other) noexcept
{
    if (this == &other)
        return *this;
    mesh_ = other.mesh_;
    return *this;
}
