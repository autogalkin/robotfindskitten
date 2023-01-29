#include "projectile.h"

#include "../core/notepader.h"

projectile& projectile::operator=(const projectile& other)
{
    if (this == &other)
        return *this;
    mesh_actor::operator =(other);
    return *this;
}

projectile& projectile::operator=(projectile&& other) noexcept
{
    if (this == &other)
        return *this;
    mesh_actor::operator =(std::move(other));
    return *this;
}



void projectile::move(const translation& offset)
{
    auto& level = notepader::get().get_world()->get_level();
    if(level->is_in_buffer(get_position()))
    {
        level->at(level->global_position_to_buffer_position(get_position())) = mesh_actor::whitespace;
    }
    
    add_position(offset);
    
    if(level->is_in_buffer(get_position()))
    {
        level->at(level->global_position_to_buffer_position(get_position())) = getmesh();
    }
    
}

void projectile::tick()
{
    if(lifetime_ > 0)
    {
        move(direction_);
        lifetime_ --;
    }
    else
    {
        notepader::get().get_world()->get_level()->destroy_actor(get_id()); 
    }
    
}
