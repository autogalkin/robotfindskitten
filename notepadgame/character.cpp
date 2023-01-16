

#include <algorithm>
#include "character.h"




#ifdef  max
#undef max
#endif

void character::h_move(const move direction)
{
    auto& wrld = notepad::get().get_world();
    const LRESULT ci = wrld->get_caret_index();
    static std::wstring s { 2, emptymesh};
    
    switch (direction){
    case move::forward:
        {
            wrld->set_selection(ci-1, ci+1);
            s[0] = emptymesh; s[1] = getmesh();
            SendMessage(wrld->get_native_window(), EM_REPLACESEL, 0, reinterpret_cast<LPARAM>(s.c_str()));
            break;
        }
        
    case move::backward:
        {
            wrld->set_selection(ci-2, ci);
            s[0] = getmesh(); s[1] = emptymesh;
            
            SendMessage(wrld->get_native_window(), EM_REPLACESEL, 0, reinterpret_cast<LPARAM>(s.c_str()));
            wrld->set_caret_index(ci-1);
            break;
        }
    }
}


void character::v_move(move direction)
{
    
    auto& wrld = notepad::get().get_world();

    LRESULT ci = wrld->get_caret_index();
    wrld->set_selection(ci-1, ci);
    
    SendMessage(wrld->get_native_window(), EM_REPLACESEL, 0, reinterpret_cast<LPARAM>(&emptymesh));
    const LRESULT cur_len = wrld->get_caret_index_in_line();
    const LRESULT next_len = wrld->get_line_lenght(wrld->get_first_char_index_in_line(wrld->get_line_index() - static_cast<LRESULT>(direction)));
    const LRESULT len = std::max(cur_len - next_len , static_cast<LRESULT>(0));
    auto add = std::wstring(len + 1, L' ');
    const std::wstring ch_body{getmesh()}; // fix LPARAM bugs  
    switch (direction) {
    case move::forward: // up
        {
            SendMessage(wrld->get_native_window(), WM_KEYDOWN, VK_UP, 0);
            Sleep(10);
            if(len > 0)
            {
                add.back() = getmesh();
                wrld->append_at_caret_position(add);
            }
            else
            {
                //on the filled field
                ci = wrld->get_caret_index();
                wrld->set_selection(ci-1, ci);
                SendMessage(wrld->get_native_window(), EM_REPLACESEL, 0, reinterpret_cast<LPARAM>(ch_body.c_str()));
                
            }
            break; 
        }
    
    case move::backward: // down
        {
            if( wrld->get_line_index() != wrld->get_lines_count()-1)
            {
                SendMessage(wrld->get_native_window(), WM_KEYDOWN, VK_DOWN, 0);
                if(len > 0)
                {
                    add.back() = getmesh();
                    wrld->append_at_caret_position(add);
                }
                else
                {
                    ci = wrld->get_caret_index();
                    wrld->set_selection(ci-1, ci);
                    SendMessage(wrld->get_native_window(), EM_REPLACESEL, 0, reinterpret_cast<LPARAM>(ch_body.c_str()));
                    
                }
            }
            else
            {
                // move to the unfilled field
                const LRESULT cur = wrld->get_caret_index_in_line();
                add.resize(cur);
                add.assign(cur,emptymesh);
                add[0] = '\n';
                add += ch_body;
                wrld->append_at_caret_position(add);
            }
            break;
        }
    }
    
}

