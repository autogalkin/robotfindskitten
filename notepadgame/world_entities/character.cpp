

#include <algorithm>
#include "character.h"

#include "projectile.h"
#include "../input.h"
#include "../core/notepader.h"


#ifdef  max
#undef max
#endif


void character::bind_input() const
{
    
    auto& input = notepader::get().get_input_manager();
    input->get_down_signal().connect([this](const input::key_state_type& state)
    {
        translation transform;
        for(auto& key : state)
        {
            switch (key)
            {
                case input::key::w: transform.line() -= 1; break;
                case input::key::a: transform.index_in_line() -= 1; break;
                case input::key::s: transform.line() += 1; break;
                case input::key::d: transform.index_in_line() += 1; break;
                case input::key::space:
                {
                    notepader::get().get_world()->get_level()->spawn_actor<projectile>(get_position() + translation{0, 1},translation{0, 1}, 15 );
                    break;
                }
                default: break;
            }
        }
        if(!transform.is_null())
        {
            notepader::get().get_world()->get_level()->set_actor_location(get_id(), get_position() + transform); 
        }
        
    }  );
}

bool character::on_hit(const actor* const other)
{
    //gamelog::cout(other->getmesh());
    return true;
}




