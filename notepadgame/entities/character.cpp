

#include <algorithm>
#include "character.h"

#include "projectile.h"
#include "../core/input.h"
#include "../core/notepader.h"
#include "../core/engine.h"
#include "world.h"
#ifdef  max
#undef max
#endif


void character::bind_input() const
{
    
    auto& input = notepader::get().get_input_manager();
    input->get_down_signal().connect([this](const input::key_state_type& state)
    {
        location transform;
        for(auto& key : state)
        {
            switch (key)
            {
                case input::key::w: transform.line -= 1; break;
                case input::key::a: transform.index_in_line -= 1; break;
                case input::key::s: transform.line += 1; break;
                case input::key::d: transform.index_in_line += 1; break;
                case input::key::space:
                {
                    notepader::get().get_world()->get_world()->spawn_actor<projectile>(get_pivot() + location{0, 1},location{0, 1}, 15 );
                    break;
                }
                default: break;  // NOLINT(clang-diagnostic-covered-switch-default)
            }
        }
        if(!transform.isZero())
        {
            notepader::get().get_world()->get_world()->set_actor_location(get_id(), get_pivot() + transform); 
        }
        
    }  );
}

bool character::on_hit(const actor* const other)
{
    //gamelog::cout(other->getmesh());
    return true;
}




