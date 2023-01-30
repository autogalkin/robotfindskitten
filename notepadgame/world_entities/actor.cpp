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




bool actor::on_hit(const actor * const other)
{
    return true;
}

void actor::connect_to_collision() const
{
    notepader::get().get_world()->get_level()->get_collision().get_query()->connect(
        [this](const actor::tag_type asker, const translation& position) -> std::optional<actor::collision_response>
        {
            if(asker != get_id() && get_position() == position)
            {
                return get_id();
            }
            return std::nullopt;
        });
}

