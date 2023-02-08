#include "projectile.h"

#include "../core/notepader.h"
#include "../core/engine.h"
#include "../core/world.h"


void projectile::tick()
{
    if(lifetime_ > 0)
    {
        notepader::get().get_world()->get_world()->set_actor_location(get_id(), get_pivot() + direction_);
        --lifetime_ ;
    }
    else
    {
        notepader::get().get_world()->get_world()->destroy_actor(get_id()); 
    }
    
}
