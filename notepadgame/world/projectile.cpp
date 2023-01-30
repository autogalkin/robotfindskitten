#include "projectile.h"

#include "../core/notepader.h"




void projectile::tick()
{
    if(lifetime_ > 0)
    {
        notepader::get().get_world()->get_level()->set_actor_location(get_id(), get_position() + direction_);
        lifetime_ --;
    }
    else
    {
        notepader::get().get_world()->get_level()->destroy_actor(get_id()); 
    }
    
}
