#pragma once


#include <algorithm>
#include <string>
#include <Windows.h>
#include <CommCtrl.h>
#include <memory>

#include <Richedit.h>
#include <vector>

#include "core/iat_hook.h"


#include "scintilla/include/Scintilla.h"


#include "core/outlog.h"



#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif



// this is the EDIT Control window of the notepad :)
class world final
{
    friend class notepader;
    friend class movement_processor;
public:

    virtual ~world()
    {
        
        if(native_dll_) FreeLibrary(native_dll_);
    }
   
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
    void insert_text(int64_t index, const std::string& s) const noexcept         { dcall2(SCI_INSERTTEXT, index, reinterpret_cast<sptr_t>(s.data()));}
    
    void show_spaces(const bool enable) const noexcept;
    void show_eol(const bool enable) const noexcept; 
    [[nodiscard]] wchar_t get_char_at_index(const int64_t index) const noexcept;
    void cursor_to_end() const noexcept;
    void set_background_color(const COLORREF c) const noexcept;
    void set_all_text_color(COLORREF c) const noexcept;
    [[nodiscard]] const HWND& get_native_window() const noexcept {return edit_window_;}
    void append_at_caret_position(const std::string& s) const noexcept;
    [[nodiscard]] int64_t get_caret_index() const noexcept;
    [[nodiscard]] int64_t get_caret_index_in_line() const noexcept;
    void set_caret_index(const int64_t index) const noexcept;
    [[nodiscard]] int64_t get_line_index(const int64_t any_char_index_in_expected_line) const noexcept;
    [[nodiscard]] int64_t get_line_lenght(const int64_t line_index ) const noexcept;
    int64_t get_current_line_text(std::wstring& out) const noexcept;
    [[nodiscard]] int64_t get_first_char_index_in_line(const int64_t line_number) const noexcept;
    [[nodiscard]] int64_t get_last_char_index_in_line(const int64_t line_number) const noexcept;
    [[nodiscard]] int64_t get_lines_count() const noexcept;
    [[nodiscard]] int64_t get_last_line_length() const noexcept;
    void get_line_text(int64_t line_index, std::vector<wchar_t> buffer) const noexcept;
    [[nodiscard]] int64_t get_all_text_length() const noexcept;
    [[nodiscard]] int64_t get_selected_text_lenght() const noexcept;
    void get_all_text(std::vector<char> buffer) const noexcept;
    void set_selection(const int64_t start, const int64_t end) const noexcept;
    void select_text_end() const noexcept;
    [[nodiscard]] std::pair<int64_t, int64_t> get_selection_range() const noexcept;
    std::pair<int64_t, int64_t> get_selection_text(std::string& out) const noexcept;
    std::pair<int64_t, int64_t> get_selection_text(std::vector<char>& out) const noexcept;
    void clear_selection(const int64_t caret_pos) const;
    void replace_selection(const std::string& new_str) const noexcept;
    void replace_selection(std::vector<char>& new_str) const noexcept;
    void set_new_all_text(const std::string& new_text) const noexcept;
    // between -10 and 50
    void set_zoom(const int zoom) const noexcept;
    

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
};

class backbuffer
{
public:
    explicit backbuffer(const std::shared_ptr<world>& world): world_(world){}
    
    const std::unique_ptr< std::vector< std::vector<char> > >& get_buffer(){return buffer;}
    
    bool init()
    {
        
        const auto w = world_.lock();
        const int width = w->get_char_width();
        
        if(RECT rect; w->get_window_rect(rect))
        {
            line_lenght_ = static_cast <int> (std::floor((std::abs(rect.left - rect.right) / width)));
            lines_count_ = w->get_lines_on_screen();
            if(!buffer) buffer = std::make_unique<std::vector< std::vector<char> >>(lines_count_);
            else buffer->resize(lines_count_);
            for(auto& line : *buffer)
            {
                line.resize(line_lenght_+2, ' '); // 2 for \n and \0
                line.back() = '\0';
            }
            return true;
        }
        return false;
    }

    void send() const;
    void set_world(const std::shared_ptr<world> w){world_ = w;}
    std::weak_ptr<world>& get_world(){return world_;}
    void get() const;

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
    PostMessage(get_native_window(), SCI_STYLESETFORE, STYLE_DEFAULT,c);  // set back-color of window
    PostMessage(get_native_window(), SCI_STYLESETFORE, STYLE_LINENUMBER, c);  // set back-color of margin
    PostMessage(get_native_window(), SCI_STYLESETFORE, SC_CHARSET_DEFAULT, c);  // char back
    PostMessage(get_native_window(), SCI_STYLESETFORE, SC_CHARSET_ANSI, c); 
    PostMessage(get_native_window(), SCI_STYLESETFORE, SC_CHARSET_SYMBOL, c);
    PostMessage(get_native_window(), SCI_STYLESETFORE, 0, c);  
}

inline std::pair<int64_t, int64_t> world::get_selection_range() const noexcept
{
    //SendMessage(get_native_window(), SCI_GETSELECTIONSTART, 0, 0),
    //SendMessage(get_native_window(), SCI_GETSELECTIONEND, 0, 0));
    auto start = dcall0(SCI_GETSELECTIONSTART);
    auto end = dcall0(SCI_GETSELECTIONEND);
    return std::make_pair(start, end);
}

inline std::pair<int64_t, int64_t> world::get_selection_text(std::string& out) const noexcept
{
    const auto range = get_selection_range();
    out.reserve(range.second - range.first);
    dcall1_l(SCI_GETSELTEXT, reinterpret_cast<sptr_t>(out.data()));
    //SendMessage(get_native_window(), SCI_GETSELTEXT, 0, reinterpret_cast<sptr_t>(out.c_str()));
    return range; 
}

inline std::pair<int64_t, int64_t> world::get_selection_text(std::vector<char>& out) const noexcept
{
    const auto range = get_selection_range();
    out.reserve(range.second - range.first);
    dcall1_l(SCI_GETSELTEXT, reinterpret_cast<sptr_t>(out.data()));
    //SendMessage(get_native_window(), SCI_GETSELTEXT, 0, reinterpret_cast<sptr_t>(out.c_str()));
    return range; 
}

inline void world::set_selection(const int64_t start, const int64_t end) const noexcept
{
    dcall2(SCI_SETSEL, start, end );
    //SendMessage(get_native_window(), SCI_SETSEL, start, end );
}

inline void world::select_text_end() const noexcept
{
    set_selection(-1 ,-1);
}

inline int64_t world::get_all_text_length() const noexcept
{
    return  dcall0(SCI_GETTEXTLENGTH); //SendMessage(get_native_window(), SCI_GETTEXTLENGTH , 0, 0);
}

inline void world::append_at_caret_position(const std::string& s) const noexcept
{
    dcall2(SCI_ADDTEXT, s.size(), reinterpret_cast<sptr_t>(s.data()));
    //SendMessage(get_native_window(), SCI_ADDTEXT, s.size(), reinterpret_cast<LPARAM>(s.data()));
}

inline int64_t world::get_selected_text_lenght() const noexcept
{
   return dcall0(SCI_GETSELTEXT); //SendMessage(get_native_window(), SCI_GETSELTEXT, 0, 0);
}

inline int64_t world::get_caret_index() const noexcept
{
    return dcall0(SCI_GETCURRENTPOS); //SendMessage(get_native_window(), SCI_GETCURRENTPOS, 0, 0);
}

inline void world::clear_selection(const int64_t caret_pos) const
{
    dcall1(SCI_SETEMPTYSELECTION, caret_pos);  //SendMessage(get_native_window(), SCI_SETEMPTYSELECTION, caret_pos, 0);
}

inline void world::set_caret_index(const int64_t index) const noexcept
{
    dcall1(SCI_GOTOPOS, index);     //SendMessage(get_native_window(),  SCI_GOTOPOS, index, 0);
}

inline int64_t world::get_line_index(const int64_t any_char_index_in_expected_line) const noexcept
{
    return  dcall1(SCI_LINEFROMPOSITION, any_char_index_in_expected_line);
    // SendMessage(get_native_window(), SCI_LINEFROMPOSITION, any_char_index_in_expected_line,  0);
}

inline int64_t world::get_line_lenght(const int64_t line_index) const noexcept
{
    return  dcall1(SCI_LINELENGTH, line_index );
    //return SendMessage( get_native_window(), SCI_LINELENGTH, line_index , 0);
}

inline int64_t world::get_current_line_text(std::wstring& out) const noexcept
{
    const int64_t len = dcall0(SCI_GETCURLINE) ;//SendMessage( get_native_window(), SCI_GETCURLINE, 0 , 0);
    out.reserve(len);
    return  dcall2(SCI_GETCURLINE, len , reinterpret_cast<sptr_t>(out.data()));
    //SendMessage( get_native_window(), SCI_GETCURLINE, len , reinterpret_cast<LPARAM>(out.data()));
    
}
inline int64_t world::get_first_char_index_in_line(const int64_t line_number) const noexcept
{
    return dcall1(SCI_POSITIONFROMLINE, line_number);
    //return SendMessage(get_native_window(), SCI_POSITIONFROMLINE, line_number, 0 );
}

inline int64_t world::get_last_char_index_in_line(const int64_t line_number) const noexcept
{
    return dcall1(SCI_GETLINEENDPOSITION, line_number);
    //return   SendMessage(get_native_window(), SCI_GETLINEENDPOSITION, line_number, 0 );
}

inline int64_t world::get_lines_count() const noexcept
{
    return dcall0(SCI_GETLINECOUNT);
   // return SendMessage(get_native_window(), SCI_GETLINECOUNT , 0, 0);
}

inline void world::replace_selection(const std::string& new_str) const noexcept
{
    dcall1_l(SCI_REPLACESEL,  reinterpret_cast<sptr_t>(new_str.data()));
    //SendMessage(get_native_window(), SCI_REPLACESEL, 0, reinterpret_cast<LPARAM>(new_str.data()) );
    
}
inline void world::replace_selection(std::vector<char>& new_str) const noexcept
{
    new_str.back() = '\0';
    dcall1_l(SCI_REPLACESEL,  reinterpret_cast<sptr_t>(new_str.data()));
    //SendMessage(get_native_window(), SCI_REPLACESEL, 0, reinterpret_cast<LPARAM>(new_str.data()) );
    
}

inline void world::set_new_all_text(const std::string& new_text) const noexcept
{
    dcall1_l(SCI_SETTEXT,  reinterpret_cast<sptr_t>(new_text.data()));
    //SendMessage(get_native_window(), SCI_SETTEXT, 0, reinterpret_cast<LPARAM>(new_text.c_str()) );
}

inline void world::set_zoom(const int zoom) const noexcept
{
    dcall1(SCI_SETZOOM,  std::clamp(zoom, -10, 50));
    //SendMessage(get_native_window(), SCI_SETZOOM,  std::clamp(zoom, -10, 50), 0);
}


inline int64_t world::get_caret_index_in_line() const noexcept
{
    const auto ci = get_caret_index();
    return ci - get_first_char_index_in_line(get_line_index(ci));
}

inline int64_t world::get_last_line_length() const noexcept
{
    return get_line_lenght(get_lines_count()-1);
}

inline wchar_t world::get_char_at_index(const int64_t index) const noexcept
{
    return static_cast<wchar_t>(dcall1(SCI_GETCHARAT, index));
    //return SendMessage(get_native_window(), SCI_GETCHARAT, index, 0);
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
    //SendMessage(get_native_window(), SCI_GETLINE, line_index, reinterpret_cast<LPARAM>( buffer.data()) );
}

inline  void world::get_all_text(std::vector<char> buffer) const noexcept
{
    const int64_t len = get_all_text_length();
    buffer.reserve(len+1);
    //SendMessage(get_native_window(), SCI_GETTEXT, len+1, reinterpret_cast<LPARAM>(buffer.data()));
    dcall2(SCI_GETTEXT, len+1, reinterpret_cast<sptr_t>( buffer.data()) );
}

inline void world::show_eol(const bool enable) const noexcept
{
    PostMessage(get_native_window(), SCI_SETVIEWEOL, enable ,0 );
}

inline void world::show_spaces(const bool enable) const noexcept
{
    int flag = SCWS_INVISIBLE;
    if(enable) flag = SCWS_VISIBLEALWAYS;
    PostMessage(get_native_window(), SCI_SETVIEWWS, flag ,0 );
}
