

#include <algorithm>
#include "character.h"
#include "../input.h"
#include "../core/notepader.h"


#ifdef  max
#undef max
#endif


void character::bind_input()
{
    connections.reserve(4);
    auto& input = notepader::get().get_input_manager();
    connections.push_back(  input->get_signals().on_d.connect([this](const input::direction d)
    {
        if(d == input::direction::down) h_move(direction::forward);
    }  ));
    connections.push_back(  input->get_signals().on_a.connect([this](const input::direction d){ if(d == input::direction::down) h_move(direction::backward); }  ));
    connections.push_back( input->get_signals().on_w.connect([this](const input::direction d){ if(d == input::direction::down) v_move(direction::backward);  }      ));
    connections.push_back(  input->get_signals().on_s.connect([this](const input::direction d){ if(d == input::direction::down) v_move(direction::forward);    }  ));
}

void character::del_old()
{
    movable::del_old();
}

bool character::spawn_new(const int64_t dest)
{
    auto& w = notepader::get().get_world();
    
    w->set_selection(dest-1, dest);
    static std::string s {2, ' '};
    auto [st, end] = w->get_selection_text(s);
    if(s[0] == entities::whitespace || end-st == 0 || s[0] == '\n')
    {
        s[1] = s[0] == '\n' ? '\n' : '\0';
        s[0] = getmesh();
        w->replace_selection(s);
        return true;
    }
    return false;
}

void character::h_move(const direction d)
{
    outlog::get().print("move");
    const int64_t p = notepader::get().get_world()->get_caret_index();
    set_position(p);
    const int64_t new_pos = std::max(p + static_cast<int64_t>(d), 0LL);
    auto& w = notepader::get().get_world();
    if(spawn_new(new_pos))
    {
        del_old();
        set_position(new_pos);
        
    }
    w->clear_selection(get_position());
    SendMessage(w->get_native_window(), SCI_SCROLLCARET , 0 , 0);
    
}


void character::v_move(direction d)
{
    int64_t p = notepader::get().get_world()->get_caret_index();
    set_position(p);
    
    auto& w = notepader::get().get_world();
    const int64_t li = w->get_line_index(p);
    const int64_t cur_len = p - w->get_first_char_index_in_line(li);
    const int64_t lines_count = w->get_lines_count();
    
    // down and last line
    if(d == direction::forward && li == lines_count-1)
    {
        del_old();
        auto new_str = std::string( cur_len+1, entities::whitespace);
        w->select_text_end();
        new_str[0] = '\n';
        new_str.back() = getmesh();
        w->replace_selection(new_str);
        set_position(w->get_caret_index());
        return;
    }
    
    
    const int64_t next_len = w->get_line_lenght(li + static_cast<int64_t>(d));
    if(next_len == 0) return; // up and first line
    
    const int64_t len = std::max(cur_len - next_len , 0LL);
    
    int64_t new_pos =  w->get_first_char_index_in_line(li + static_cast<int64_t>(d)) + cur_len;
    
    if(len == 0)
    {
        if(spawn_new(new_pos))
        {
            del_old();
            set_position(new_pos);
        }
        w->clear_selection(get_position());
    }
    else
    {
        del_old();
        auto new_str = std::string( len+2, entities::whitespace);
        new_str[len] = getmesh();
        new_str.back() = (li == lines_count-2 )? '\n' : '\0';
        w->set_selection(new_pos - len-1, new_pos - len-1);
        w->replace_selection(new_str);
        w->clear_selection(new_pos);
        set_position(new_pos);
    }
}

