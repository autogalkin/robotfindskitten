#pragma once


#include <algorithm>
#include <string>
#include <Windows.h>
#include <CommCtrl.h>




// this is the EDIT Control window of the notepad :)
class world final
{
    friend class notepad;
public:
    explicit world(const HWND& edit_window): edit_window_(edit_window)
    {}
    
    HWND& get_native_window(){return edit_window_;}
    void append_at_caret_position(const std::wstring& s);
    void add_char_at_caret_position(int count=1, const wchar_t c=L' ') const;

    // current position from 0
    LRESULT get_caret_index();
    LRESULT get_caret_index_in_line();
    
    //  from 0 to last char
    void set_caret_index(const WPARAM index);
    LRESULT get_line_index(const WPARAM any_char_index_in_expected_line = -1);

    // https://learn.microsoft.com/en-us/windows/win32/controls/em-linelength
    // input may be char index from EM_LINEFROMCHAR or caret index, -1 - the current caret line. 
    LRESULT get_line_lenght(const WPARAM any_char_index_in_expected_line = -1);

    // native call : EM_LINEINDEX
    // A value of -1 specifies the current line number (the line that contains the caret).
    // 0 = first line
    LRESULT get_first_char_index_in_line(const WPARAM line_number = -1);
    LRESULT get_lines_count();
    LRESULT get_last_line_length();
    //The character index of a character in the line whose length is to be retrieved.
    //If this parameter is greater than the number of characters in the control, the return value is zero.
    //  -1 - the current caret line(in simple cases)
    std::wstring get_line_text(const WPARAM any_char_index_in_expected_line = -1);
    LRESULT get_all_text_length();
    std::wstring get_all_text();
    
    void set_selection(const WPARAM start_index, const LPARAM stop_index);
    void replace_selection(const std::wstring& new_str);
    void set_new_all_text(const std::wstring& new_text);
    // between 1 and 50, 1 -is defaul 100%
    void set_zoom(const int zoom, const int default_value=1);
    
private:
    HWND edit_window_{};
    
    // change the background color and text color, used  inside the WndProc function
    static LRESULT set_window_color(const HDC& device_context);
};






// inlines

inline LRESULT world::get_all_text_length()
{
    return SendMessage(get_native_window(), WM_GETTEXTLENGTH, 0, 0);
}

inline LRESULT world::get_caret_index_in_line()
{
    return get_caret_index() - get_first_char_index_in_line();
}

inline LRESULT world::get_last_line_length()
{
    return get_line_lenght(get_all_text_length());
}

inline void world::add_char_at_caret_position(int count, const wchar_t c) const
{
    PostMessage(edit_window_, WM_CHAR, c,  std::clamp(count, 0, 15));
}

inline LRESULT world::get_caret_index()
{
    return SendMessage(get_native_window(), EM_GETCARETINDEX, 0, 0);
}

inline void world::set_caret_index(const WPARAM index)
{
    SendMessage(get_native_window(), EM_SETCARETINDEX, index, 0);
}

inline LRESULT world::get_line_index(const WPARAM any_char_index_in_expected_line)
{
    return SendMessage(get_native_window(), EM_LINEFROMCHAR, any_char_index_in_expected_line , 0);
}

inline LRESULT world::get_line_lenght(const WPARAM any_char_index_in_expected_line)
{
    return SendMessage( get_native_window(), EM_LINELENGTH, any_char_index_in_expected_line , 0);
}

inline LRESULT world::get_first_char_index_in_line(const WPARAM line_number)
{
    return  SendMessage(get_native_window(), EM_LINEINDEX, line_number, 0 );
}

inline LRESULT world::get_lines_count()
{
    return SendMessage(get_native_window(), EM_GETLINECOUNT, 0, 0);
}

inline void world::set_selection(const WPARAM start_index, const LPARAM stop_index)
{
    SendMessage(get_native_window(), EM_SETSEL, start_index , stop_index);
}

inline  void world::replace_selection(const std::wstring& new_str)
{
    SendMessage(get_native_window(), EM_REPLACESEL, TRUE, reinterpret_cast<LPARAM>(new_str.c_str()) );
}

inline  void world::set_new_all_text(const std::wstring& new_text)
{
    SendMessage(get_native_window(), WM_SETTEXT, 0, reinterpret_cast<LPARAM>(new_text.c_str()) );
}

inline void world::set_zoom(const int zoom, const int default_value)
{
    SendMessage(get_native_window(), EM_SETZOOM,  std::clamp(zoom, 0, 50), std::clamp(default_value, 0, 50));
}