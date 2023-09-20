#pragma warning(push, 0)
#include "Windows.h"
#include <CommCtrl.h>
#include <Richedit.h>
#pragma warning(pop)
#include "engine.h"
#include "notepader.h"
#include "world.h"

engine::~engine() = default;
HWND engine::create_native_window(const DWORD dwExStyle,
                                  const LPCWSTR lpWindowName,
                                  const DWORD dwStyle, const int X, const int Y,
                                  const int nWidth, const int nHeight,
                                  const HWND hWndParent, const HMENU hMenu,
                                  const HINSTANCE hInstance,
                                  const LPVOID lpParam) {
    edit_window_ =
        CreateWindowExW(dwExStyle, L"Scintilla", lpWindowName, dwStyle, X, Y,
                        nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);

    init_direct_access();

    dcall2(SCI_STYLESETFONT, STYLE_DEFAULT,
           reinterpret_cast<sptr_t>("Lucida Console"));
    dcall2(SCI_STYLESETBOLD, STYLE_DEFAULT, 1); // bold
    dcall2(SCI_STYLESETSIZE, STYLE_DEFAULT, 16); // pt size
    // dcall2(SCI_STYLESETCHECKMONOSPACED, STYLE_DEFAULT,1);
    dcall2(SCI_SETHSCROLLBAR, 1, 0);

    // hide control symbol mnemonics
    dcall1(SCI_SETCONTROLCHARSYMBOL, ' ');

    show_spaces(notepader::options::show_spaces & start_options_ ? 1 : 0);
    show_eol(notepader::options::show_eol & start_options_ ? 1 : 0);

    world_ = std::make_unique<class world>(this);

    world_->init(nWidth, nHeight);

    return edit_window_;
}

void engine::init_direct_access() {
    direct_function_ =
        reinterpret_cast<npi_t(__cdecl*)(sptr_t, int, uptr_t, sptr_t)>(
            GetProcAddress(native_dll_.get(), "Scintilla_DirectFunction"));
    // direct_function_ = reinterpret_cast<npi_t (__cdecl *)(sptr_t, int,
    // uptr_t, sptr_t)>(SendMessage(get_native_window(),SCI_GETDIRECTFUNCTION,
    // 0, 0));
    direct_wnd_ptr_ =
        SendMessage(get_native_window(), SCI_GETDIRECTPOINTER, 0, 0);
    // thread_safe_function_ =  reinterpret_cast<int64_t (__cdecl *)(sptr_t,
    // int, uptr_t, sptr_t)>(&SendMessageW); //
    // NOLINT(clang-diagnostic-cast-function-type)
}

uint32_t engine::get_window_width() const noexcept {
    RECT r;
    get_window_rect(r);
    return r.right - r.left;
}
