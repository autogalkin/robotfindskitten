#include "actor.h"
#include "../core/notepader.h"
#include "level.h"

int64_t translation::to_index() noexcept
{
    return  notepader::get().get_world()->get_first_char_index_in_line( line() ) + index_in_line();
}

translation translation::from_index(const int64_t index) noexcept
{
    auto& w = notepader::get().get_world();
    return {w->get_line_index(index), index - w->get_first_char_index_in_line( w->get_line_index(index)) };
}


void actor::connect_to_collision(collision* collision)
{
    collision_query_connection = collision->get_query().connect([this](const actor::tag_type asker, const translation& position)
    {
        return collision_query_responce(asker,  position);
    });
            
}

std::optional<actor::collision_response> actor::collision_query_responce(const actor::tag_type asker, const translation& position) const
{
    if(asker != get_id() && get_position() == position)
    {
        return get_id();
    }
    return std::nullopt;
}

bool actor::on_hit(const actor * const other)
{
    return true;
}

