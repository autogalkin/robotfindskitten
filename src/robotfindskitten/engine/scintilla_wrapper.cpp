#include "Windows.h"
#include <CommCtrl.h>
#include <Richedit.h>

#include "engine/scintilla_wrapper.h"
#include "engine/notepad.h"

scintilla::~scintilla() = default;
HWND scintilla::create_native_window(const DWORD dwExStyle,
                                     const LPCWSTR lpWindowName,
                                     const DWORD dwStyle, const int X,
                                     const int Y, const int nWidth,
                                     const int nHeight, HWND hWndParent,
                                     HMENU hMenu, HINSTANCE hInstance,
                                     LPVOID lpParam, uint8_t start_options) {
    edit_window_ =
        CreateWindowExW(dwExStyle, L"Scintilla", lpWindowName, dwStyle, X, Y,
                        nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);

    init_direct_access_to_scintilla();

    dcall2(SCI_STYLESETFONT, STYLE_DEFAULT,
           reinterpret_cast<sptr_t>("Lucida Console"));
    dcall2(SCI_STYLESETBOLD, STYLE_DEFAULT, 1); // bold
    dcall2(SCI_STYLESETITALIC, STYLE_DEFAULT, 1); // italic
    static constexpr int FONT_SIZE = 16;
    dcall2(SCI_STYLESETSIZE, STYLE_DEFAULT, FONT_SIZE); // pt size
    // dcall2(SCI_STYLESETCHECKMONOSPACED, STYLE_DEFAULT,1);
    dcall1(SCI_SETHSCROLLBAR, 0);
    dcall1(SCI_SETVSCROLLBAR, 0);

    // TODO(Igor): atmosphere actor cannot change this color
    if(notepad::opts::hide_selection & start_options) {
        dcall2(SCI_SETELEMENTCOLOUR, SC_ELEMENT_SELECTION_BACK,
               RGB(255, 255, 255));
    }

    dcall1(SCI_SETSELEOLFILLED, 0);

    ShowScrollBar(edit_window_, SB_BOTH, FALSE);
    // hide control symbol mnemonics
    dcall1(SCI_SETCONTROLCHARSYMBOL, ' ');
    show_spaces((notepad::opts::show_spaces & start_options) != 0);
    show_eol((notepad::opts::show_eol & start_options) != 0);

    return edit_window_;
}

void scintilla::init_direct_access_to_scintilla() {
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

uint32_t scintilla::get_window_width() const noexcept {
    RECT r = get_window_rect();
    return r.right - r.left;
}

void scintilla::set_background_color(const COLORREF c) const noexcept {
    PostMessage(get_native_window(), SCI_STYLESETBACK, STYLE_DEFAULT,
                c); // set back-color of window
    PostMessage(get_native_window(), SCI_STYLESETBACK, STYLE_LINENUMBER,
                c); // set back-color of margin
    PostMessage(get_native_window(), SCI_STYLESETBACK, SC_CHARSET_DEFAULT,
                c); // char back
    PostMessage(get_native_window(), SCI_STYLESETBACK, SC_CHARSET_ANSI, c); //
    PostMessage(get_native_window(), SCI_STYLESETBACK, SC_CHARSET_SYMBOL, c); //
    PostMessage(get_native_window(), SCI_STYLESETBACK, 0, c); //
}

void scintilla::force_set_background_color(const COLORREF c) const noexcept {
    // TODO(Igor): selection color if selection enabled
    // if(notepad::opts::hide_selection & start_options){
    dcall2(SCI_SETELEMENTCOLOUR, SC_ELEMENT_SELECTION_BACK, c);

    // dcall2(SCI_STYLESETBACK, STYLE_DEFAULT, c);
    // dcall2(SCI_STYLESETBACK, STYLE_LINENUMBER, c);
    // dcall2(SCI_STYLESETBACK, SC_CHARSET_DEFAULT, c);
    // dcall2(SCI_STYLESETBACK, SC_CHARSET_ANSI, c);
    dcall2(SCI_STYLESETBACK, STYLE_DEFAULT, c);
    dcall2(SCI_STYLESETBACK, 0, c);
}

void scintilla::set_all_text_color(const COLORREF c) const noexcept {
    PostMessage(get_native_window(), SCI_STYLESETFORE, STYLE_DEFAULT,
                c); // set back-color of window
    PostMessage(get_native_window(), SCI_STYLESETFORE, STYLE_LINENUMBER,
                c); // set back-color of margin
    PostMessage(get_native_window(), SCI_STYLESETFORE, SC_CHARSET_DEFAULT,
                c); // char back
    PostMessage(get_native_window(), SCI_STYLESETFORE, SC_CHARSET_ANSI, c);
    PostMessage(get_native_window(), SCI_STYLESETFORE, SC_CHARSET_SYMBOL, c);
    PostMessage(get_native_window(), SCI_STYLESETFORE, 0, c);
}

void scintilla::force_set_all_text_color(const COLORREF c) const noexcept {
    dcall2(SCI_STYLESETFORE, STYLE_DEFAULT, c);
    dcall2(SCI_STYLESETFORE, STYLE_LINENUMBER, c);
    dcall2(SCI_STYLESETFORE, SC_CHARSET_DEFAULT, c);
    dcall2(SCI_STYLESETFORE, SC_CHARSET_ANSI, c);
    dcall2(SCI_STYLESETFORE, SC_CHARSET_SYMBOL, c);
    dcall2(SCI_STYLESETFORE, 0, c);
}

template<is_container_of_chars T>
std::pair<npi_t, npi_t> scintilla::get_selection_text(T& out) const noexcept {
    const auto range = get_selection_range();
    out.reserve(range.second - range.first);
    dcall1_l(SCI_GETSELTEXT, reinterpret_cast<sptr_t>(out.data()));
    return range;
}

npi_t scintilla::get_caret_index_in_line() const noexcept {
    const auto ci = get_caret_index();
    return ci - get_first_char_index_in_line(get_line_index(ci));
}

template<is_container_of_chars T>
void scintilla::get_line_text(const npi_t line_index,
                              T& buffer) const noexcept {
    const npi_t line_length = get_line_lenght(line_index);
    buffer.reserve(line_length + 1);
    dcall2(SCI_GETLINE, line_index, reinterpret_cast<sptr_t>(buffer.data()));
}

template<is_container_of_chars T>
void scintilla::get_all_text(T& buffer) const noexcept {
    const npi_t len = get_all_text_length();
    buffer.reserve(len + 1);
    dcall2(SCI_GETTEXT, len + 1, reinterpret_cast<sptr_t>(buffer.data()));
}

void scintilla::show_spaces(const bool enable) const noexcept {
    int flag = SCWS_INVISIBLE;
    if(enable) {
        flag = SCWS_VISIBLEALWAYS;
    }
    PostMessage(get_native_window(), SCI_SETVIEWWS, flag, 0);
}
