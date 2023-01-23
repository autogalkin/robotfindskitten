#include "world.h"
#include <CommCtrl.h>
#include <Richedit.h>
#include "boost/range/adaptor/indexed.hpp"
#include "core/notepader.h"


HWND world::create_native_window(const DWORD dwExStyle, const LPCWSTR lpWindowName, const DWORD dwStyle,
                                 const int X, const int Y, const int nWidth, const int nHeight, const HWND hWndParent, const HMENU hMenu,
                                 const HINSTANCE hInstance, const LPVOID lpParam)
{
    native_dll_ = LoadLibrary(TEXT("Scintilla.dll"));
    
    edit_window_ = CreateWindowEx(dwExStyle,
    L"Scintilla",lpWindowName, dwStyle,
    X,Y,nWidth,nHeight,hWndParent,hMenu, hInstance,lpParam);
    init_direct_access();

    dcall2( SCI_STYLESETFONT, STYLE_DEFAULT, reinterpret_cast<sptr_t>("Lucida Console"));
    dcall2(SCI_STYLESETBOLD, STYLE_DEFAULT, 1); // bold
    dcall2(SCI_STYLESETSIZE, STYLE_DEFAULT,36); // pt size
    dcall2(SCI_STYLESETCHECKMONOSPACED, STYLE_DEFAULT,1);
    dcall2(SCI_SETHSCROLLBAR, 1, 0);

    backbuffer = std::make_unique<class backbuffer>(this);
    backbuffer->init(nWidth);
    return edit_window_;
}

void world::init_direct_access()
{
    direct_function_ = reinterpret_cast<int64_t (__cdecl *)(sptr_t, int, uptr_t, sptr_t)>(SendMessage(get_native_window(),SCI_GETDIRECTFUNCTION, 0, 0));
    direct_wnd_ptr_ = SendMessage(get_native_window(),SCI_GETDIRECTPOINTER, 0, 0); 
    thread_safe_function_ =  reinterpret_cast<int64_t (__cdecl *)(sptr_t, int, uptr_t, sptr_t)>(&SendMessageW); // NOLINT(clang-diagnostic-cast-function-type)
}

void backbuffer::init(const int window_width)
{
    //RECT rect; world_->get_window_rect(rect)
   // static_cast <int> (std::floor((std::abs(rect.left - rect.right) / width)))
    const int char_width = world_->get_char_width();
    line_lenght_ = window_width / char_width;
    lines_count_ = world_->get_lines_on_screen();
    if(!buffer) buffer = std::make_unique<std::vector< std::vector<char> >>(lines_count_);
    else buffer->resize(lines_count_);
    for(auto& line : *buffer)
    {
        line.resize(line_lenght_+2, ' '); // 2 for \n and \0
        line.back() = '\0';
    }

}

void backbuffer::send() const
{
    const auto pos = world_->get_caret_index();
    const int64_t fl = world_->get_first_visible_line();
    const int64_t h_scroll = world_->get_horizontal_scroll_offset() / world_->get_char_width();
    
    for(const auto [i, line] : *buffer | boost::adaptors::indexed(0))
    {
        const int64_t lnum = fl + i;
        const int64_t fi = world_->get_first_char_index_in_line(lnum);
        const int64_t ll = world_->get_line_lenght(lnum); // include endl if exists
        int64_t endsel = h_scroll + line_lenght_+endl > ll-endl ? ll : line_lenght_ + h_scroll+1;
           
        char ch_end {'\0'};
        if(lnum >= world_->get_lines_count()-1)    { ch_end = '\n';   }
        else                                  { endsel  -= endl; }
            
        *(line.end() - 1/*past-the-end*/ - 1 /* '\0' */) = ch_end; 
        world_->set_selection(fi+h_scroll,  fi  + endsel);
        world_->replace_selection(line);
    }
    world_->set_caret_index(pos);
}

void backbuffer::get() const
{
    const auto pos = world_->get_caret_index();
    const int64_t fl = world_->get_first_visible_line();
    const int64_t h_scroll = world_->get_horizontal_scroll_offset() / world_->get_char_width();
    
    for(const auto [i, line] : *buffer | boost::adaptors::indexed(0))
    {
        const int64_t lnum = fl + i;
        const int64_t fi = world_->get_first_char_index_in_line(lnum);
        //const int64_t ll = w->get_line_lenght(lnum);
        
        world_->set_selection(fi+h_scroll,  fi  + line_lenght_ + h_scroll);
        //Sleep(500);
        auto [st, end] = world_->get_selection_text(line);
    }
    world_->set_caret_index(pos);
}



