#include "world.h"

#include <algorithm>
#include <CommCtrl.h>
#include <vector>

void world::append_at_caret_position(const std::wstring& s)
{
    const LRESULT end = get_caret_index()-1;
    SendMessage(edit_window_, EM_SETSEL, end, end); // set selection - end of text
    SendMessage(edit_window_, EM_REPLACESEL, 0, reinterpret_cast<LPARAM>(s.c_str())); // append!
        
}


std::wstring world::get_line_text(const WPARAM any_char_index_in_expected_line)
{
    const LRESULT current_line_num = get_line_index(any_char_index_in_expected_line);
    const LRESULT line_length = get_line_lenght(any_char_index_in_expected_line);
    std::vector<wchar_t> w_buffer(line_length+1, '\0');
    wchar_t* buffer_ptr = w_buffer.data();
    *reinterpret_cast<unsigned short*>( buffer_ptr  ) = static_cast<unsigned short>( line_length+1);
    SendMessage(get_native_window(), EM_GETLINE, current_line_num, reinterpret_cast<LPARAM>( buffer_ptr) );
    return {w_buffer.begin(), w_buffer.end()};
}

std::wstring world::get_all_text()
{
    const LRESULT len = get_all_text_length();
    std::vector<wchar_t> buffer(len+1, '\0');
    SendMessage(get_native_window(), WM_GETTEXT, len+1, reinterpret_cast<LPARAM>(buffer.data()));
    return {buffer.begin(), buffer.end()}; 
}


LRESULT world::set_window_color(const HDC& device_context)
{
    //SetBkMode( device_context, TRANSPARENT ); // some beautiful bugs!
    SetTextColor(device_context, RGB(241, 241, 241));
    SetBkColor(device_context, RGB(37, 37, 38));
    //(LRESULT) GetStockObject(DC_BRUSH); for only text
    return reinterpret_cast<LRESULT>(CreateSolidBrush(RGB(37, 37, 38)));
}
