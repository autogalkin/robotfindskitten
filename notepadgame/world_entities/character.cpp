

#include <algorithm>
#include "character.h"

#include "projectile.h"

#include "../input.h"
#include "../core/notepader.h"


#ifdef  max
#undef max
#endif


character& character::operator=(const character& other)
{
    if (this == &other)
        return *this;
    mesh_actor::operator =(other);
    return *this;
}

character& character::operator=(character&& other) noexcept
{
    if (this == &other)
        return *this;
    input_connection = std::move(other.input_connection);
    mesh_actor::operator =(std::move(other));
    return *this;
}

void character::bind_input()
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
        move(transform);
        
    }  );
}

void character::move(const translation& offset)
{
    if(offset.is_null()) return;
    
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



