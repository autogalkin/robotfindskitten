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
    backbuffer = std::make_unique<class backbuffer>(notepader::get().get_world()); // TODO как правильно передать world
    return edit_window_;
}

void world::init_direct_access()
{
     //direct_function_ = reinterpret_cast<int64_t (__cdecl *)(sptr_t, int, uptr_t, sptr_t)>( 
         //GetProcAddress(  native_dll_, "Scintilla_DirectFunction"));
    direct_function_ = reinterpret_cast<int64_t (__cdecl *)(sptr_t, int, uptr_t, sptr_t)>(SendMessage(get_native_window(),SCI_GETDIRECTFUNCTION, 0, 0));
    //direct_function_ =  reinterpret_cast<int64_t (__cdecl *)(sptr_t, int, uptr_t, sptr_t)>(&SendMessageW); // NOLINT(clang-diagnostic-cast-function-type)
    //direct_wnd_ptr_ = reinterpret_cast<sptr_t>(edit_window_);
    direct_wnd_ptr_ = static_cast<sptr_t>(SendMessage(get_native_window(),SCI_GETDIRECTPOINTER, 0, 0));
}

bool backbuffer::init()
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

void backbuffer::send() const
{
    const auto w = world_.lock();
    const auto pos = w->get_caret_index();
    const int64_t fl = w->get_first_visible_line();
    const int64_t h_scroll = w->get_horizontal_scroll_offset() / w->get_char_width();
        
    for(const auto [i, line] : *buffer | boost::adaptors::indexed(0))
    {
        const int64_t lnum = fl + i;
        const int64_t fi = w->get_first_char_index_in_line(lnum);
        const int64_t ll = w->get_line_lenght(lnum); // include endl if exists
        int64_t endsel = h_scroll + line_lenght_+endl > ll-endl ? ll : line_lenght_ + h_scroll+1;
           
        char ch_end {'\0'};
        if(lnum >= w->get_lines_count()-1)    { ch_end = '\n';   }
        else                                  { endsel  -= endl; }
            
        *(line.end() - 1/*past-the-end*/ - endl) = ch_end; 
        w->set_selection(fi+h_scroll,  fi  + endsel);
        w->replace_selection(line);
    }
       
    w->set_caret_index(pos);
}

void backbuffer::get() const
{
    const auto w = world_.lock();
    const auto pos = w->get_caret_index();
    const int64_t fl = w->get_first_visible_line();
    const int64_t h_scroll = w->get_horizontal_scroll_offset() / w->get_char_width();
    
    for(const auto [i, line] : *buffer | boost::adaptors::indexed(0))
    {
        const int64_t lnum = fl + i;
        const int64_t fi = w->get_first_char_index_in_line(lnum);
        //const int64_t ll = w->get_line_lenght(lnum);
        
        w->set_selection(fi+h_scroll,  fi  + line_lenght_ + h_scroll);
        //Sleep(500);
        auto [st, end] = w->get_selection_text(line);
    }
    w->set_caret_index(pos);
}



