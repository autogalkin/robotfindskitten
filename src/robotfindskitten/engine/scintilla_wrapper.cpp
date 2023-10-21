#include <Windows.h>
#include <CommCtrl.h>
#include <Richedit.h>

#include "engine/notepad.h"
#include "engine/scintilla_wrapper.h"
namespace scintilla {

HWND scintilla_dll::create_native_window(DWORD dwExStyle, LPCWSTR lpWindowName,
                                     DWORD dwStyle, int X, int Y, int nWidth,
                                     int nHeight, HWND hWndParent, HMENU hMenu,
                                     HINSTANCE hInstance, LPVOID lpParam,
                                     uint8_t start_options) {
    edit_window_ =
        ::CreateWindowExW(dwExStyle, L"Scintilla", lpWindowName, dwStyle, X, Y,
                        nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);

    init_direct_access_to_scintilla();

    dcall2(SCI_STYLESETFONT, STYLE_DEFAULT,
           reinterpret_cast<sptr_t>("Lucida Console"));
    dcall2(SCI_STYLESETBOLD, STYLE_DEFAULT, 1); // bold
    dcall2(SCI_STYLESETITALIC, STYLE_DEFAULT, 1); // italic
    static constexpr int FONT_SIZE = 16;
    dcall2(SCI_STYLESETSIZE, STYLE_DEFAULT, FONT_SIZE); // pt size
    dcall1_w(SCI_SETHSCROLLBAR, 0);
    dcall1_w(SCI_SETVSCROLLBAR, 0);

    if(::notepad::opts::hide_selection & start_options) {
        dcall2(SCI_SETELEMENTCOLOUR, SC_ELEMENT_SELECTION_BACK,
               0);
    }

    dcall1_w(SCI_SETSELEOLFILLED, 0);

    ::ShowScrollBar(edit_window_, SB_BOTH, FALSE);
    // Hide control symbol mnemonics
    dcall1_w(SCI_SETCONTROLCHARSYMBOL, ' ');
    look_op{*this}.show_spaces((::notepad::opts::show_spaces & start_options) != 0);
    look_op{*this}.show_eol((::notepad::opts::show_eol & start_options) != 0);

    return edit_window_;
}

void scintilla_dll::init_direct_access_to_scintilla() {
    direct_function_ =
        reinterpret_cast<npi_t(__cdecl*)(sptr_t, int, uptr_t, sptr_t)>(
            ::GetProcAddress(native_dll_.get(), "Scintilla_DirectFunction"));
    direct_wnd_ptr_ =
        ::SendMessage(get_native_window(), SCI_GETDIRECTPOINTER, 0, 0);
}

} // namespace scintilla
