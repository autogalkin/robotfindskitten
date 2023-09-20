#pragma once

#pragma warning(push, 0)
#include "Scintilla.h"
#include "boost/signals2/signal.hpp"
// clang-format off
#include <Richedit.h>
#include <Windows.h>
#include <CommCtrl.h>
// clang-format on
#include <algorithm>
#include <memory>
#include <string>
#pragma warning(pop)
#include "details/base_types.h"
#include "details/nonconstructors.h"
#include "world.h"

template <typename T>
concept is_container_of_chars = requires(T t) {
    { t.data() } -> std::convertible_to<char*>;
};

// this is the EDIT Control window of the notepad
class engine final
    : public noncopyable,
      public nonmoveable // Deleted so engine objects can not be copied.
{
    friend class notepader;

  public:
    // Only the notepader object can create the engine
    engine() = delete;

    using scroll_changed_signal =
        boost::signals2::signal<void(const position&)>;
    using size_changed_signal =
        boost::signals2::signal<void(uint32_t width, uint32_t height)>;

    [[nodiscard]] HWND get_native_window() const noexcept {
        return edit_window_;
    }
    [[nodiscard]] const std::unique_ptr<world>& get_world() { return world_; }
    [[nodiscard]] scroll_changed_signal& get_on_scroll_changed() {
        return on_scroll_changed_;
    }
    [[nodiscard]] size_changed_signal& get_on_resize() { return on_resize_; }
    [[nodiscard]] const position& get_scroll() const { return scroll_; }
    void scroll(const npi_t columns_to_scroll,
                const npi_t lines_to_scroll) const {
        dcall2(SCI_LINESCROLL, columns_to_scroll, lines_to_scroll);
    }
    // caret
    [[nodiscard]] npi_t get_caret_index() const noexcept {
        return dcall0(SCI_GETCURRENTPOS);
    }
    [[nodiscard]] npi_t get_caret_index_in_line() const noexcept;
    void set_caret_index(const npi_t index) const noexcept {
        dcall1(SCI_GOTOPOS, index);
    }
    void caret_to_end() const noexcept {
        set_caret_index(get_all_text_length());
    }
    void append_at_caret_position(const std::string& s) const noexcept {
        dcall2(SCI_ADDTEXT, s.size(), reinterpret_cast<sptr_t>(s.data()));
    }

    void insert_text(const npi_t index, const std::string& s) const noexcept {
        dcall2(SCI_INSERTTEXT, index, reinterpret_cast<sptr_t>(s.data()));
    }

    // selection
    void set_selection(const npi_t start, const npi_t end) const noexcept {
        dcall2(SCI_SETSEL, start, end);
    }
    void clear_selection(const npi_t caret_pos) const {
        dcall1(SCI_SETEMPTYSELECTION, caret_pos);
    }
    void select_text_end() const noexcept { set_selection(-1, -1); }
    [[nodiscard]] std::pair<npi_t, npi_t> get_selection_range() const noexcept {
        return std::make_pair(dcall0(SCI_GETSELECTIONSTART),
                              dcall0(SCI_GETSELECTIONEND));
    }
    void replace_selection(const std::string_view new_str) const noexcept {
        dcall1_l(SCI_REPLACESEL, reinterpret_cast<sptr_t>(new_str.data()));
    }
    void set_new_all_text(const std::string& new_text) const noexcept {
        dcall1_l(SCI_SETTEXT, reinterpret_cast<sptr_t>(new_text.c_str()));
    }

    // getters
    [[nodiscard]] npi_t get_horizontal_scroll_offset() const noexcept {
        return dcall0(SCI_GETXOFFSET);
    }
    [[nodiscard]] char get_char_at_index(const npi_t index) const noexcept {
        return static_cast<char>(dcall1(SCI_GETCHARAT, index));
    }
    [[nodiscard]] npi_t
    get_first_char_index_in_line(const npi_t line_number) const noexcept {
        return dcall1(SCI_POSITIONFROMLINE, line_number);
    }
    [[nodiscard]] npi_t
    get_last_char_index_in_line(const npi_t line_number) const noexcept {
        return dcall1(SCI_GETLINEENDPOSITION, line_number);
    }
    [[nodiscard]] npi_t get_line_lenght(const npi_t line_index) const noexcept {
        return dcall1(SCI_LINELENGTH, line_index);
    }
    [[nodiscard]] npi_t
    get_line_index(const npi_t any_char_index_in_expected_line) const noexcept {
        return dcall1(SCI_LINEFROMPOSITION, any_char_index_in_expected_line);
    }

    // size
    // pixels
    [[nodiscard]] uint8_t get_char_width() const noexcept {
        return static_cast<uint8_t>(dcall2(SCI_TEXTWIDTH, STYLE_DEFAULT,
                                           reinterpret_cast<sptr_t>(" ")));
    }
    // pixels
    [[nodiscard]] uint8_t get_line_height() const noexcept {
        return static_cast<uint8_t>(dcall1(SCI_TEXTHEIGHT, STYLE_DEFAULT));
    }
    // pixels
    bool get_window_rect(RECT& outrect) const noexcept {
        return GetClientRect(get_native_window(), &outrect);
    }
    [[nodiscard]] uint32_t get_window_width() const noexcept;
    [[nodiscard]] uint8_t get_font_size() const noexcept /*size in pt*/ {
        return static_cast<uint8_t>(dcall1(SCI_STYLEGETSIZE, STYLE_DEFAULT));
    }
    [[nodiscard]] npi_t get_first_visible_line() const noexcept {
        return dcall0(SCI_GETFIRSTVISIBLELINE);
    }
    [[nodiscard]] uint8_t get_lines_on_screen() const noexcept {
        return static_cast<uint8_t>(dcall0(SCI_LINESONSCREEN));
    }
    [[nodiscard]] npi_t get_selected_text_lenght() const noexcept {
        return dcall0(SCI_GETSELTEXT);
    }
    [[nodiscard]] npi_t get_all_text_length() const noexcept {
        return dcall0(SCI_GETTEXTLENGTH);
    }
    [[nodiscard]] npi_t get_lines_count() const noexcept {
        return dcall0(SCI_GETLINECOUNT);
    }
    [[nodiscard]] npi_t get_last_line_length() const noexcept {
        return get_line_lenght(get_lines_count() - 1);
    }

    template <is_container_of_chars T>
    void get_line_text(npi_t line_index, T& buffer) const noexcept;
    template <is_container_of_chars T>
    void get_all_text(T& buffer) const noexcept;
    template <is_container_of_chars T>
    std::pair<npi_t, npi_t> get_selection_text(T& out) const noexcept;

    // look dev
    void set_background_color(const COLORREF c) const noexcept; // post message
    void
    force_set_background_color(const COLORREF c) const noexcept; // send message
    [[nodiscard]] COLORREF get_background_color() const noexcept {
        return static_cast<COLORREF>(dcall1(SCI_STYLESETBACK, STYLE_DEFAULT));
    }
    void set_all_text_color(COLORREF c) const noexcept; // post message
    void force_set_all_text_color(COLORREF c) const noexcept; // send message
    void show_spaces(const bool enable) const noexcept;
    void show_eol(const bool enable) const noexcept {
        PostMessage(get_native_window(), SCI_SETVIEWEOL, enable, 0);
    }
    // between -10 and 50
    void set_zoom(const int zoom) const noexcept {
        dcall1(SCI_SETZOOM, std::clamp(zoom, -10, 50));
    }
    [[nodiscard]] int get_zoom() const noexcept {
        return static_cast<int>(dcall0(SCI_GETZOOM));
    }

    ~engine();

  protected:
    // ~Constructor, call once from the notepad class
    explicit engine(const uint8_t init_options = 0 /*notepad::options*/)
        : start_options_(init_options),
          native_dll_{LoadLibrary(TEXT("Scintilla.dll")), &::FreeLibrary} {}
    // help to create unique ptr
    [[nodiscard]] static std::unique_ptr<engine>
    create_new(const uint8_t init_options = 0) {
        return std::unique_ptr<engine>{new engine(init_options)};
    }

    HWND create_native_window(DWORD dwExStyle, LPCWSTR lpWindowName,
                              DWORD dwStyle, int X, int Y, int nWidth,
                              int nHeight, HWND hWndParent, HMENU hMenu,
                              HINSTANCE hInstance, LPVOID lpParam);

    void set_h_scroll(const npi_t p) {
        scroll_.index_in_line() = p;
        on_scroll_changed_(scroll_);
    }
    void set_v_scroll(const npi_t p) {
        scroll_.line() = p;
        on_scroll_changed_(scroll_);
    }

    void init_direct_access();

    using direct_function = npi_t (*)(sptr_t, int, uptr_t, sptr_t);

    npi_t dcall0(const int message) const {
        return direct_function_(direct_wnd_ptr_, message, 0, 0);
    } // NOLINT(modernize-use-nodiscard)
    npi_t dcall1(const int message, const uptr_t w) const {
        return direct_function_(direct_wnd_ptr_, message, w, 0);
    } // NOLINT(modernize-use-nodiscard)
    npi_t dcall1_l(const int message, const sptr_t l) const {
        return direct_function_(direct_wnd_ptr_, message, 0, l);
    } // NOLINT(modernize-use-nodiscard)
    npi_t dcall2(const int message, const uptr_t w, const sptr_t l) const {
        return direct_function_(direct_wnd_ptr_, message, w, l);
    } // NOLINT(modernize-use-nodiscard)

  private:
    uint8_t start_options_;

    HWND edit_window_{nullptr};
    std::unique_ptr<std::remove_pointer_t<HMODULE>, decltype(&::FreeLibrary)>
        native_dll_;

    std::unique_ptr<world> world_{nullptr};

    sptr_t direct_wnd_ptr_{0};
    npi_t (*direct_function_)(sptr_t, int, uptr_t, sptr_t){nullptr};

    position scroll_{};
    scroll_changed_signal on_scroll_changed_{};
    size_changed_signal on_resize_{};
};

// inlines

inline void engine::set_background_color(const COLORREF c) const noexcept {
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

inline void
engine::force_set_background_color(const COLORREF c) const noexcept {
    dcall2(SCI_STYLESETBACK, STYLE_DEFAULT, c);
    dcall2(SCI_STYLESETBACK, STYLE_LINENUMBER, c);
    dcall2(SCI_STYLESETBACK, SC_CHARSET_DEFAULT, c);
    dcall2(SCI_STYLESETBACK, SC_CHARSET_ANSI, c);
    dcall2(SCI_STYLESETBACK, 0, c);
}

inline void engine::set_all_text_color(const COLORREF c) const noexcept {
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

inline void engine::force_set_all_text_color(const COLORREF c) const noexcept {
    dcall2(SCI_STYLESETFORE, STYLE_DEFAULT, c);
    dcall2(SCI_STYLESETFORE, STYLE_LINENUMBER, c);
    dcall2(SCI_STYLESETFORE, SC_CHARSET_DEFAULT, c);
    dcall2(SCI_STYLESETFORE, SC_CHARSET_ANSI, c);
    dcall2(SCI_STYLESETFORE, SC_CHARSET_SYMBOL, c);
    dcall2(SCI_STYLESETFORE, 0, c);
}

template <is_container_of_chars T>
std::pair<npi_t, npi_t> engine::get_selection_text(T& out) const noexcept {
    const auto range = get_selection_range();
    out.reserve(range.second - range.first);
    dcall1_l(SCI_GETSELTEXT, reinterpret_cast<sptr_t>(out.data()));
    return range;
}

inline npi_t engine::get_caret_index_in_line() const noexcept {
    const auto ci = get_caret_index();
    return ci - get_first_char_index_in_line(get_line_index(ci));
}

template <is_container_of_chars T>
void engine::get_line_text(const npi_t line_index, T& buffer) const noexcept {
    const npi_t line_length = get_line_lenght(line_index);
    buffer.reserve(line_length + 1);
    dcall2(SCI_GETLINE, line_index, reinterpret_cast<sptr_t>(buffer.data()));
}

template <is_container_of_chars T>
void engine::get_all_text(T& buffer) const noexcept {
    const npi_t len = get_all_text_length();
    buffer.reserve(len + 1);
    dcall2(SCI_GETTEXT, len + 1, reinterpret_cast<sptr_t>(buffer.data()));
}

inline void engine::show_spaces(const bool enable) const noexcept {
    int flag = SCWS_INVISIBLE;
    if (enable)
        flag = SCWS_VISIBLEALWAYS;
    PostMessage(get_native_window(), SCI_SETVIEWWS, flag, 0);
}
