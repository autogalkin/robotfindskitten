#include "actor.h"
#include "../core/notepader.h"
void movable::del_old()
{
    auto p = get_position();
    auto& w = notepader::get().get_world();
    w->set_selection(p-1, p);
    w->replace_selection(std::string{entities::whitespace});
}

bool movable::spawn_new(const int64_t dest)
{
    auto& w = notepader::get().get_world();
    w->set_selection(dest-1, dest);
    w->replace_selection(std::string{getmesh()});
    return true;
}
