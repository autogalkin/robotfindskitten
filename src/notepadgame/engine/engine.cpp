#pragma warning(push, 0)
#include "Windows.h"
#include <CommCtrl.h>
#include <Richedit.h>
#pragma warning(pop)
#include "engine.h"
#include "notepad.h"
#include "world.h"

engine_t::~engine_t() = default;
HWND engine_t::create_native_window(const DWORD dwExStyle,
                                  const LPCWSTR lpWindowName,
                                  const DWORD dwStyle, const int X, const int Y,
                                  const int nWidth, const int nHeight,
                                  const HWND hWndParent, const HMENU hMenu,
                                  const HINSTANCE hInstance,
                                  const LPVOID lpParam, uint8_t start_options) {
    edit_window_ =
        CreateWindowExW(dwExStyle, L"Scintilla", lpWindowName, dwStyle, X, Y,
                        nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);

    init_direct_access_to_scintilla();

    dcall2(SCI_STYLESETFONT, STYLE_DEFAULT,
           reinterpret_cast<sptr_t>("Lucida Console"));
    dcall2(SCI_STYLESETBOLD, STYLE_DEFAULT, 1); // bold
    dcall2(SCI_STYLESETSIZE, STYLE_DEFAULT, 16); // pt size
    // dcall2(SCI_STYLESETCHECKMONOSPACED, STYLE_DEFAULT,1);
    dcall2(SCI_SETHSCROLLBAR, 1, 0);

    // hide control symbol mnemonics
    dcall1(SCI_SETCONTROLCHARSYMBOL, ' ');

    show_spaces(notepad_t::opts::show_spaces & start_options ? 1 : 0);
    show_eol(notepad_t::opts::show_eol & start_options ? 1 : 0);

    world_.emplace(this);
    world_->backbuffer.init(nWidth / get_char_width(), nHeight / get_line_height());

    return edit_window_;
}

void engine_t::init_direct_access_to_scintilla() {
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

uint32_t engine_t::get_window_width() const noexcept {
    RECT r;
    get_window_rect(r);
    return r.right - r.left;
}



void engine_t::set_background_color(const COLORREF c) const noexcept {
  PostMessage(get_native_window(), SCI_STYLESETBACK, STYLE_DEFAULT,
              c); // set back-color of window
  PostMessage(get_native_window(), SCI_STYLESETBACK, STYLE_LINENUMBER,
              c); // set back-color of margin
  PostMessage(get_native_window(), SCI_STYLESETBACK, SC_CHARSET_DEFAULT,
              c); // char back
  PostMessage(get_native_window(), SCI_STYLESETBACK, SC_CHARSET_ANSI, c);   //
  PostMessage(get_native_window(), SCI_STYLESETBACK, SC_CHARSET_SYMBOL, c); //
  PostMessage(get_native_window(), SCI_STYLESETBACK, 0, c);                 //
}

void
engine_t::force_set_background_color(const COLORREF c) const noexcept {
  dcall2(SCI_STYLESETBACK, STYLE_DEFAULT, c);
  dcall2(SCI_STYLESETBACK, STYLE_LINENUMBER, c);
  dcall2(SCI_STYLESETBACK, SC_CHARSET_DEFAULT, c);
  dcall2(SCI_STYLESETBACK, SC_CHARSET_ANSI, c);
  dcall2(SCI_STYLESETBACK, 0, c);
}

void engine_t::set_all_text_color(const COLORREF c) const noexcept {
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

void engine_t::force_set_all_text_color(const COLORREF c) const noexcept {
  dcall2(SCI_STYLESETFORE, STYLE_DEFAULT, c);
  dcall2(SCI_STYLESETFORE, STYLE_LINENUMBER, c);
  dcall2(SCI_STYLESETFORE, SC_CHARSET_DEFAULT, c);
  dcall2(SCI_STYLESETFORE, SC_CHARSET_ANSI, c);
  dcall2(SCI_STYLESETFORE, SC_CHARSET_SYMBOL, c);
  dcall2(SCI_STYLESETFORE, 0, c);
}

template <is_container_of_chars T>
std::pair<npi_t, npi_t> engine_t::get_selection_text(T &out) const noexcept {
  const auto range = get_selection_range();
  out.reserve(range.second - range.first);
  dcall1_l(SCI_GETSELTEXT, reinterpret_cast<sptr_t>(out.data()));
  return range;
}

npi_t engine_t::get_caret_index_in_line() const noexcept {
  const auto ci = get_caret_index();
  return ci - get_first_char_index_in_line(get_line_index(ci));
}

template <is_container_of_chars T>
void engine_t::get_line_text(const npi_t line_index, T &buffer) const noexcept {
  const npi_t line_length = get_line_lenght(line_index);
  buffer.reserve(line_length + 1);
  dcall2(SCI_GETLINE, line_index, reinterpret_cast<sptr_t>(buffer.data()));
}

template <is_container_of_chars T>
void engine_t::get_all_text(T &buffer) const noexcept {
  const npi_t len = get_all_text_length();
  buffer.reserve(len + 1);
  dcall2(SCI_GETTEXT, len + 1, reinterpret_cast<sptr_t>(buffer.data()));
}

void engine_t::show_spaces(const bool enable) const noexcept {
  int flag = SCWS_INVISIBLE;
  if (enable)
    flag = SCWS_VISIBLEALWAYS;
  PostMessage(get_native_window(), SCI_SETVIEWWS, flag, 0);
}
