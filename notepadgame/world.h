#pragma once


#include <algorithm>
#include <string>
#include <Windows.h>
#include <CommCtrl.h>
#include <memory>

#include <Richedit.h>
#include <vector>

#include "scintilla/include/Scintilla.h"


#include "core/outlog.h"



#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif


template<typename T>
concept Container = requires(T t)
{
    std::begin(t);
    std::end(t);
};

class backbuffer;

// this is the EDIT Control window of the notepad :)
class world final
{
    friend class notepader;
public:

    virtual ~world()
    {
        if(native_dll_) FreeLibrary(native_dll_);
    }
    
    [[nodiscard]] const HWND& get_native_window() const noexcept                 { return edit_window_;}
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
    [[nodiscard]] int64_t get_lines_count() const noexcept                       {return dcall0(SCI_GETLINECOUNT);}
    [[nodiscard]] int64_t get_last_line_length() const noexcept                  {return get_line_lenght(get_lines_count()-1);}
    void set_new_all_text(const std::string& new_text) const noexcept            { dcall1_l(SCI_SETTEXT,  reinterpret_cast<sptr_t>(new_text.data()));}
    void clear_selection(const int64_t caret_pos) const                          { dcall1(SCI_SETEMPTYSELECTION, caret_pos);}
    // between -10 and 50
    void set_zoom(const int zoom) const noexcept                                 { dcall1(SCI_SETZOOM,  std::clamp(zoom, -10, 50));}
    [[nodiscard]] int64_t get_first_char_index_in_line(const int64_t line_number) const noexcept       {return dcall1(SCI_POSITIONFROMLINE, line_number);}
    [[nodiscard]] int64_t get_last_char_index_in_line(const int64_t line_number) const noexcept        {return dcall1(SCI_GETLINEENDPOSITION, line_number);}
    [[nodiscard]] int64_t get_line_lenght(const int64_t line_index) const noexcept                     {return  dcall1(SCI_LINELENGTH, line_index );}
    [[nodiscard]] int64_t get_line_index(const int64_t any_char_index_in_expected_line) const noexcept { return dcall1(SCI_LINEFROMPOSITION, any_char_index_in_expected_line);}
    void show_spaces(const bool enable) const noexcept;
    void cursor_to_end() const noexcept;
    void set_background_color(const COLORREF c) const noexcept;
    void set_all_text_color(COLORREF c) const noexcept;
    
    [[nodiscard]] int64_t get_caret_index_in_line() const noexcept;
    int64_t get_current_line_text(std::wstring& out) const noexcept;
    void get_line_text(int64_t line_index, std::vector<wchar_t> buffer) const noexcept;
    void get_all_text(std::vector<char> buffer) const noexcept;
    
    [[nodiscard]] std::pair<int64_t, int64_t> get_selection_range() const noexcept;

    template<typename T>
    requires Container
    void get_selection_t(T& out) const noexcept  {}

    std::pair<int64_t, int64_t> get_selection_text(std::string& out) const noexcept;
    std::pair<int64_t, int64_t> get_selection_text(std::vector<char>& out) const noexcept;
    
    void replace_selection(const std::string& new_str) const noexcept;
    void replace_selection(std::vector<char>& new_str) const noexcept;

private:
    // for the notepad class
    HWND create_native_window(DWORD dwExStyle, LPCWSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU
                              hMenu, HINSTANCE hInstance, LPVOID lpParam);
    
    void init_direct_access();
    int64_t dcall0(const int message) const                                   { return direct_function_(direct_wnd_ptr_, message, 0, 0);}
    int64_t dcall1(const int message, const uptr_t w) const                   { return direct_function_(direct_wnd_ptr_, message, w, 0);}
    int64_t dcall1_l(const int message, const sptr_t l) const                 { return direct_function_(direct_wnd_ptr_, message, 0, l);}
    int64_t dcall2(const int message, const uptr_t w, const sptr_t l) const   { return direct_function_(direct_wnd_ptr_, message, w, l);}
    
    HWND edit_window_{nullptr};
    HMODULE native_dll_{nullptr};
    sptr_t direct_wnd_ptr_{0};
    int64_t(*direct_function_)(sptr_t, int, uptr_t, sptr_t){nullptr};
    std::unique_ptr<backbuffer> backbuffer{nullptr};
};


class backbuffer
{
public:
    friend class world;
    explicit backbuffer(const std::shared_ptr<world>& owner): world_(owner){}

    
    [[nodiscard]] char& at(const int64_t line, const int64_t index) const
    {
        return (*buffer)[line][index];
    }
    bool init();
    void send() const;
    void get() const;
    
    [[nodiscard]] int get_line_size() const noexcept { return line_lenght_;} 
    [[nodiscard]] const std::unique_ptr< std::vector< std::vector<char> > >& get_buffer() const noexcept { return buffer;}
    [[nodiscard]] const std::weak_ptr<world>& get_owner_world() const noexcept { return world_;}
    void set_owner_world(const std::shared_ptr<world>& w) noexcept { world_ = w;}
    
protected:
    

private:
    static constexpr int endl = 1;
    int line_lenght_{0};
    int lines_count_{0};
    std::unique_ptr< std::vector< std::vector<char> > > buffer;
    std::weak_ptr<world> world_;
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
    std::vector<char> v;
    get_selection_t(v);
    PostMessage(get_native_window(), SCI_STYLESETFORE, STYLE_DEFAULT,c);  // set back-color of window
    PostMessage(get_native_window(), SCI_STYLESETFORE, STYLE_LINENUMBER, c);  // set back-color of margin
    PostMessage(get_native_window(), SCI_STYLESETFORE, SC_CHARSET_DEFAULT, c);  // char back
    PostMessage(get_native_window(), SCI_STYLESETFORE, SC_CHARSET_ANSI, c); 
    PostMessage(get_native_window(), SCI_STYLESETFORE, SC_CHARSET_SYMBOL, c);
    PostMessage(get_native_window(), SCI_STYLESETFORE, 0, c);  
}

inline std::pair<int64_t, int64_t> world::get_selection_range() const noexcept
{
    auto start = dcall0(SCI_GETSELECTIONSTART);
    auto end = dcall0(SCI_GETSELECTIONEND);
    return std::make_pair(start, end);
}

inline std::pair<int64_t, int64_t> world::get_selection_text(std::string& out) const noexcept
{
    const auto range = get_selection_range();
    out.reserve(range.second - range.first);
    dcall1_l(SCI_GETSELTEXT, reinterpret_cast<sptr_t>(out.data()));
    return range; 
}

inline std::pair<int64_t, int64_t> world::get_selection_text(std::vector<char>& out) const noexcept
{
    const auto range = get_selection_range();
    out.reserve(range.second - range.first);
    dcall1_l(SCI_GETSELTEXT, reinterpret_cast<sptr_t>(out.data()));
    return range; 
}


inline int64_t world::get_current_line_text(std::wstring& out) const noexcept
{
    const int64_t len = dcall0(SCI_GETCURLINE) ;
    out.reserve(len);
    return  dcall2(SCI_GETCURLINE, len , reinterpret_cast<sptr_t>(out.data()));
    
}

inline void world::replace_selection(const std::string& new_str) const noexcept
{
    dcall1_l(SCI_REPLACESEL,  reinterpret_cast<sptr_t>(new_str.data()));
    
    
}
inline void world::replace_selection(std::vector<char>& new_str) const noexcept
{
    new_str.back() = '\0';
    dcall1_l(SCI_REPLACESEL,  reinterpret_cast<sptr_t>(new_str.data()));
}

inline int64_t world::get_caret_index_in_line() const noexcept
{
    const auto ci = get_caret_index();
    return ci - get_first_char_index_in_line(get_line_index(ci));
}


inline  void world::cursor_to_end() const noexcept
{
    const int64_t index = get_all_text_length();
    set_caret_index(index);
}

inline  void world::get_line_text(const int64_t line_index, std::vector<wchar_t> buffer) const noexcept
{
    const int64_t line_length = get_line_lenght(line_index);
    buffer.reserve(line_length+1);
    dcall2(SCI_GETLINE, line_index, reinterpret_cast<sptr_t>( buffer.data()) );
}

inline  void world::get_all_text(std::vector<char> buffer) const noexcept
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
