#include "world.h"
#include <CommCtrl.h>
#include <Richedit.h>

#include "notepader.h"

#include "boost/range/adaptor/indexed.hpp"
#include "../world/level.h"



world::~world() = default;
HWND world::create_native_window(const DWORD dwExStyle, const LPCWSTR lpWindowName, const DWORD dwStyle,
                                 const int X, const int Y, const int nWidth, const int nHeight, const HWND hWndParent, const HMENU hMenu,
                                 const HINSTANCE hInstance, const LPVOID lpParam)
{
    edit_window_ = CreateWindowEx(dwExStyle,
    L"Scintilla",lpWindowName, dwStyle,
    X,Y,nWidth,nHeight,hWndParent,hMenu, hInstance,lpParam);
    init_direct_access();
    
    dcall2( SCI_STYLESETFONT, STYLE_DEFAULT, reinterpret_cast<sptr_t>("Lucida Console"));
    dcall2(SCI_STYLESETBOLD, STYLE_DEFAULT, 1); // bold
    dcall2(SCI_STYLESETSIZE, STYLE_DEFAULT,36); // pt size
    dcall2(SCI_STYLESETCHECKMONOSPACED, STYLE_DEFAULT,1);
    dcall2(SCI_SETHSCROLLBAR, 1, 0);
    show_spaces(notepader::options::show_spaces & start_options_ ? 1 : 0);
    show_eol(notepader::options::show_eol & start_options_ ? 1 : 0);
    level = std::make_unique<class level>(this);
    level->init(nWidth);
    return edit_window_;
}

void world::init_direct_access()
{
    direct_function_ = reinterpret_cast<int64_t (__cdecl *)(sptr_t, int, uptr_t, sptr_t)>(SendMessage(get_native_window(),SCI_GETDIRECTFUNCTION, 0, 0));
    direct_wnd_ptr_ = SendMessage(get_native_window(),SCI_GETDIRECTPOINTER, 0, 0); 
    thread_safe_function_ =  reinterpret_cast<int64_t (__cdecl *)(sptr_t, int, uptr_t, sptr_t)>(&SendMessageW); // NOLINT(clang-diagnostic-cast-function-type)
}



