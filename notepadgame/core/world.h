#pragma once


#include <algorithm>
#include <string>
#include <Windows.h>
#include <CommCtrl.h>
#include <memory>

#include <Richedit.h>
#include <vector>

#include "Scintilla.h"
#include <boost/signals2.hpp>




#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif



template<typename T>
concept is_container_of_chars = requires(T t)
{
    {t.data()} -> std::convertible_to<char*>;
};


class level;
// this is the EDIT Control window of the notepad
class world final : public std::enable_shared_from_this<world>
{
    friend class notepader;
    
public:
    world() = delete;
    world(world &other) = delete;
    world& operator=(const world& other) = delete;
    world(world&& other) noexcept = delete;
    world& operator=(world&& other) noexcept = delete;

    
    [[nodiscard]] const HWND& get_native_window() const noexcept                 { return edit_window_;}
    [[nodiscard]] const std::unique_ptr<level>& get_level()                      { return  level;}
    void set_caret_index(const int64_t index) const noexcept                     { dcall1(SCI_GOTOPOS, index); }
    void enable_multi_selection(const bool enable) const noexcept                { dcall1(SCI_SETMULTIPLESELECTION, enable);}
    void set_multi_paste_type(const int type=SC_MULTIPASTE_EACH) const noexcept  { dcall1(SCI_SETMULTIPASTE, type);         }
    void clear_multi_selection() const noexcept                                  { dcall0(SCI_CLEARSELECTIONS);             }
    void add_selection(const int64_t start, const int64_t end) const noexcept    { dcall2(SCI_ADDSELECTION,start, end );    }
    [[nodiscard]] int64_t get_first_visible_line() const noexcept                { return dcall0(SCI_GETFIRSTVISIBLELINE);  }
    [[nodiscard]] int get_lines_on_screen() const noexcept                       { return static_cast<int>(dcall0(SCI_LINESONSCREEN));        } 
    [[nodiscard]] int get_char_width() const noexcept                            { return static_cast<int>(dcall2(SCI_TEXTWIDTH,STYLE_DEFAULT, reinterpret_cast<sptr_t>(" "))); }
    [[nodiscard]] int get_line_height() const noexcept                           { return static_cast<int>(dcall1(SCI_TEXTHEIGHT ,STYLE_DEFAULT )); }
    bool get_window_rect(RECT& outrect) const noexcept                           { return GetClientRect(get_native_window(), &outrect);}
    [[nodiscard]] int get_zoom()const noexcept                                   { return static_cast<int>(dcall0(SCI_GETZOOM));}
    [[nodiscard]] int get_font_size() const noexcept                             { return static_cast<int>(dcall1(SCI_STYLEGETSIZE, STYLE_DEFAULT));}
    [[nodiscard]] int64_t get_horizontal_scroll_offset() const noexcept          { return dcall0(SCI_GETXOFFSET);}
    void insert_text(const int64_t index, const std::string& s) const noexcept   { dcall2(SCI_INSERTTEXT, index, reinterpret_cast<sptr_t>(s.data()));}
    void show_eol(const bool enable) const noexcept                              { PostMessage(get_native_window(), SCI_SETVIEWEOL, enable ,0 );}
    [[nodiscard]] wchar_t get_char_at_index(const int64_t index) const noexcept  { return static_cast<wchar_t>(dcall1(SCI_GETCHARAT, index));}
    void append_at_caret_position(const std::string& s) const noexcept           { dcall2(SCI_ADDTEXT, s.size(), reinterpret_cast<sptr_t>(s.data()));}
    [[nodiscard]] int64_t get_caret_index() const noexcept                       { return dcall0(SCI_GETCURRENTPOS);}
    [[nodiscard]] int64_t get_all_text_length() const noexcept                   { return  dcall0(SCI_GETTEXTLENGTH);}
    [[nodiscard]] int64_t get_selected_text_lenght() const noexcept              { return dcall0(SCI_GETSELTEXT);}
    void set_selection(const int64_t start, const int64_t end) const noexcept    { dcall2(SCI_SETSEL, start, end );}
    void select_text_end() const noexcept                                        { set_selection(-1 ,-1);}
    [[nodiscard]] int64_t get_lines_count() const noexcept                       { return dcall0(SCI_GETLINECOUNT);}
    [[nodiscard]] int64_t get_last_line_length() const noexcept                  { return get_line_lenght(get_lines_count()-1);}
    void set_new_all_text(const std::string& new_text) const noexcept            { dcall1_l(SCI_SETTEXT,  reinterpret_cast<sptr_t>(new_text.c_str()));}
    void clear_selection(const int64_t caret_pos) const                          { dcall1(SCI_SETEMPTYSELECTION, caret_pos);}
    void cursor_to_end() const noexcept                                          { set_caret_index(get_all_text_length());}
    [[nodiscard]] std::pair<int64_t, int64_t> get_selection_range()const noexcept{return std::make_pair(dcall0(SCI_GETSELECTIONSTART), dcall0(SCI_GETSELECTIONEND));}
    // between -10 and 50
    void set_zoom(const int zoom) const noexcept                                 { dcall1(SCI_SETZOOM,  std::clamp(zoom, -10, 50));}
    [[nodiscard]] int64_t get_first_char_index_in_line(const int64_t line_number) const noexcept       { return dcall1(SCI_POSITIONFROMLINE, line_number);}
    [[nodiscard]] int64_t get_last_char_index_in_line(const int64_t line_number) const noexcept        { return dcall1(SCI_GETLINEENDPOSITION, line_number);}
    [[nodiscard]] int64_t get_line_lenght(const int64_t line_index) const noexcept                     { return dcall1(SCI_LINELENGTH, line_index );}
    [[nodiscard]] int64_t get_line_index(const int64_t any_char_index_in_expected_line) const noexcept { return dcall1(SCI_LINEFROMPOSITION, any_char_index_in_expected_line);}
    
    void show_spaces(const bool enable) const noexcept;
    void set_background_color(const COLORREF c) const noexcept;
    void set_all_text_color(COLORREF c) const noexcept;
    [[nodiscard]] int64_t get_caret_index_in_line() const noexcept;
    
    template<is_container_of_chars T>
    void get_line_text(int64_t line_index, T& buffer) const noexcept;
    
    template <is_container_of_chars T>
    void get_all_text(T& buffer) const noexcept;
    
    template<is_container_of_chars T>
    std::pair<int64_t, int64_t> get_selection_text(T& out) const noexcept;
    
    void replace_selection(const std::string_view new_str) const noexcept { dcall1_l(SCI_REPLACESEL,  reinterpret_cast<sptr_t>(new_str.data()));}
    
    ~world();

protected:
    
    
    // calls from the notepad class
    [[nodiscard]] static std::shared_ptr<world> make(const uint8_t start_options=0)  { return std::shared_ptr<world>{new world(start_options)};}
    HWND create_native_window(DWORD dwExStyle, LPCWSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU
                              hMenu, HINSTANCE hInstance, LPVOID lpParam);

    
    void init_direct_access();
    
    using direct_function = int64_t(*)(sptr_t, int, uptr_t, sptr_t);
    
    int64_t dcall0(const int message) const                                   { return direct_function_(direct_wnd_ptr_, message, 0, 0);}  // NOLINT(modernize-use-nodiscard)
    int64_t dcall1(const int message, const uptr_t w) const                   { return direct_function_(direct_wnd_ptr_, message, w, 0);}  // NOLINT(modernize-use-nodiscard)
    int64_t dcall1_l(const int message, const sptr_t l) const                 { return direct_function_(direct_wnd_ptr_, message, 0, l);}  // NOLINT(modernize-use-nodiscard)
    int64_t dcall2(const int message, const uptr_t w, const sptr_t l) const   { return direct_function_(direct_wnd_ptr_, message, w, l);}  // NOLINT(modernize-use-nodiscard)

private:
    uint8_t start_options_;
    explicit world(const uint8_t init_options = 0/*notepad::options*/): start_options_(init_options) {}
   
    HWND edit_window_{nullptr};
    HMODULE native_dll_{nullptr};
    std::unique_ptr<level> level{nullptr};
    sptr_t direct_wnd_ptr_{0};
    int64_t(*direct_function_)(sptr_t, int, uptr_t, sptr_t){nullptr};
    int64_t(*thread_safe_function_)(sptr_t, int, uptr_t, sptr_t){nullptr};
    
};






// inlines

inline void world::set_background_color(const COLORREF c) const noexcept
{
    PostMessage(get_native_window(), SCI_STYLESETBACK, STYLE_DEFAULT,c);  // set back-color of window
    PostMessage(get_native_window(), SCI_STYLESETBACK, STYLE_LINENUMBER, c);  // set back-color of margin
    PostMessage(get_native_window(), SCI_STYLESETBACK, SC_CHARSET_DEFAULT, c);  // char back
    PostMessage(get_native_window(), SCI_STYLESETBACK, SC_CHARSET_ANSI, c);  //
    PostMessage(get_native_window(), SCI_STYLESETBACK, SC_CHARSET_SYMBOL, c);  //
    PostMessage(get_native_window(), SCI_STYLESETBACK, 0, c);  //
}

inline void world::set_all_text_color(const COLORREF c) const noexcept
{
    PostMessage(get_native_window(), SCI_STYLESETFORE, STYLE_DEFAULT,c);  // set back-color of window
    PostMessage(get_native_window(), SCI_STYLESETFORE, STYLE_LINENUMBER, c);  // set back-color of margin
    PostMessage(get_native_window(), SCI_STYLESETFORE, SC_CHARSET_DEFAULT, c);  // char back
    PostMessage(get_native_window(), SCI_STYLESETFORE, SC_CHARSET_ANSI, c); 
    PostMessage(get_native_window(), SCI_STYLESETFORE, SC_CHARSET_SYMBOL, c);
    PostMessage(get_native_window(), SCI_STYLESETFORE, 0, c);  
}

template<is_container_of_chars T>
std::pair<int64_t, int64_t> world::get_selection_text(T& out) const noexcept
{
    const auto range = get_selection_range();
    out.reserve(range.second - range.first);
    dcall1_l(SCI_GETSELTEXT, reinterpret_cast<sptr_t>(out.data()));
    return range; 
}


inline int64_t world::get_caret_index_in_line() const noexcept
{
    const auto ci = get_caret_index();
    return ci - get_first_char_index_in_line(get_line_index(ci));
}


template<is_container_of_chars T>
void world::get_line_text(const int64_t line_index, T& buffer) const noexcept
{
    const int64_t line_length = get_line_lenght(line_index);
    buffer.reserve(line_length+1);
    dcall2(SCI_GETLINE, line_index, reinterpret_cast<sptr_t>( buffer.data()) );
}

template<is_container_of_chars T>
void world::get_all_text(T& buffer) const noexcept
{
    const int64_t len = get_all_text_length();
    buffer.reserve(len+1);
    dcall2(SCI_GETTEXT, len+1, reinterpret_cast<sptr_t>( buffer.data()) );
}


inline void world::show_spaces(const bool enable) const noexcept
{
    int flag = SCWS_INVISIBLE;
    if(enable) flag = SCWS_VISIBLEALWAYS;
    PostMessage(get_native_window(), SCI_SETVIEWWS, flag ,0 );
}
