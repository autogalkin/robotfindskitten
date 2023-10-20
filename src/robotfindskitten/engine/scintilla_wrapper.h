/**
 * @file
 * @brief Scintilla API wrapper
 */

#ifndef _CPP_PROJECTS_ROBOTFINDSKITTEN_SRC_ROBOTFINDSKITTEN_ENGINE_SCINTILLA_WRAPPER_H
#define _CPP_PROJECTS_ROBOTFINDSKITTEN_SRC_ROBOTFINDSKITTEN_ENGINE_SCINTILLA_WRAPPER_H

#include <algorithm>
#include <memory>
#include <optional>
#include <string>

#include <Scintilla.h>
#include <ILexer.h>
// clang-format off
#include <Richedit.h>
#include <Windows.h>
#include <CommCtrl.h>
// clang-format on

#include "engine/details/base_types.hpp"
#include "engine/details/nonconstructors.h"

class notepad;
bool hook_CreateWindowExW(HMODULE);

namespace scintilla {

template<typename T>
concept is_container_of_chars = requires(T t) {
    { t.data() } -> std::convertible_to<char*>;
    { t.reserve(10) };
};

/**
 * @brief Key that prevent to construct a class outside class T
 */
template<typename T>
class construct_key {
    explicit construct_key() = default;
    explicit construct_key(int /*payload*/) {} // Not an aggregate
    friend T;
};

class scintilla_dll;

/**
 * @brief Specialization of construct_key for scintilla
 *
 * Allow to create a scintilla in scintilla and in hook_CreateWindowExW
 */
template<>
class construct_key<scintilla_dll> {
    friend bool ::hook_CreateWindowExW(HMODULE);
};

// this is the EDIT Control window of the notepad
/**
 * @class scintilla
 * @brief this is wrapper for Scintilla Edit Control
 * window of the game in notepad, replacement for a standart Edit Control.
 *
 * It allow colorize word, set back and front colors, font,
 * fast push new text, scroll left and down
 *
 * A Project builds with patched Scintilla version, where we disable
 *  - EnsureCaretVisible for prevent unnesessary scrolls and flickering
 *
 */
class scintilla_dll
    : public noncopyable,
      public nonmoveable // Deleted so scintilla objects can not be copied.
{
    friend bool ::hook_CreateWindowExW(HMODULE);
    friend class ::notepad;
    std::unique_ptr<std::remove_pointer_t<HMODULE>, decltype(&::FreeLibrary)>
        native_dll_;
    HWND edit_window_{nullptr};
    sptr_t direct_wnd_ptr_{0};
    npi_t (*direct_function_)(sptr_t, int, uptr_t, sptr_t){nullptr};
    HWND create_native_window(DWORD dwExStyle, LPCWSTR lpWindowName,
                              DWORD dwStyle, int X, int Y, int nWidth,
                              int nHeight, HWND hWndParent, HMENU hMenu,
                              HINSTANCE hInstance, LPVOID lpParam,
                              uint8_t start_options);
    void init_direct_access_to_scintilla();

public:
    // Only the hook_CreateWindowExW can create this class
    explicit scintilla_dll(
        construct_key<scintilla_dll> /*access in friends*/) noexcept
        : native_dll_{LoadLibrary(TEXT("Scintilla.dll")), &::FreeLibrary} {}

    using direct_function_t = npi_t (*)(sptr_t, int, uptr_t, sptr_t);

    [[nodiscard]] HWND get_native_window() const noexcept {
        return edit_window_;
    }
    // NOLINTBEGIN(modernize-use-nodiscard)
    npi_t dcall0(int message) const {
        return direct_function_(direct_wnd_ptr_, message, 0, 0);
    }
    npi_t dcall1_w(int message, uptr_t w) const {
        return direct_function_(direct_wnd_ptr_, message, w, 0);
    }
    npi_t dcall1_l(int message, sptr_t l) const {
        return direct_function_(direct_wnd_ptr_, message, 0, l);
    }
    npi_t dcall2(int message, uptr_t w, sptr_t l) const {
        return direct_function_(direct_wnd_ptr_, message, w, l);
    }
    // NOLINTEND(modernize-use-nodiscard)
};

struct scintilla_ref {
    // Pure mans gsl::not_null
    std::reference_wrapper<scintilla_dll> dll_;

    [[nodiscard]] constexpr scintilla_dll& sc() const noexcept {
        return dll_.get();
    }
};
struct selection_op: scintilla_ref {
    void set_selection(npi_t start, npi_t end) const noexcept {
        sc().dcall2(SCI_SETSEL, start, end);
    }
    void clear_selection(npi_t caret_pos) const noexcept {
        sc().dcall1_w(SCI_SETEMPTYSELECTION, caret_pos);
    }
    void select_text_end() const noexcept {
        set_selection(-1, -1);
    }
    [[nodiscard]] std::pair<npi_t, npi_t> get_selection_range() const noexcept {
        return std::make_pair(sc().dcall0(SCI_GETSELECTIONSTART),
                              sc().dcall0(SCI_GETSELECTIONEND));
    }
    void replace_selection(std::string_view new_str) const noexcept {
        sc().dcall1_l(SCI_REPLACESEL, reinterpret_cast<sptr_t>(new_str.data()));
    }
};


struct text_op: scintilla_ref {
    void insert_text(npi_t index, std::string_view s) const noexcept {
        sc().dcall2(SCI_INSERTTEXT, index, reinterpret_cast<sptr_t>(s.data()));
    }
    void append_at_caret_position(std::string_view s) const noexcept {
        sc().dcall2(SCI_ADDTEXT, s.size(), reinterpret_cast<sptr_t>(s.data()));
    }
    void set_new_all_text(std::string_view new_text) const noexcept {
        sc().dcall1_l(SCI_SETTEXT, reinterpret_cast<sptr_t>(new_text.data()));
        sc().dcall0(SCI_EMPTYUNDOBUFFER);
    }
    [[nodiscard]] char get_char_at_index(npi_t index) const noexcept {
        return static_cast<char>(sc().dcall1_w(SCI_GETCHARAT, index));
    }
    [[nodiscard]] npi_t
    get_first_char_index_in_line(npi_t line_number) const noexcept {
        return sc().dcall1_w(SCI_POSITIONFROMLINE, line_number);
    }

    [[nodiscard]] npi_t
    get_last_char_index_in_line(npi_t line_number) const noexcept {
        return sc().dcall1_w(SCI_GETLINEENDPOSITION, line_number);
    }

    [[nodiscard]] npi_t get_line_lenght(npi_t line_index) const noexcept {
        return sc().dcall1_w(SCI_LINELENGTH, line_index);
    }

    [[nodiscard]] npi_t
    get_line_index(const npi_t any_char_index_in_expected_line) const noexcept {
        return sc().dcall1_w(SCI_LINEFROMPOSITION,
                             any_char_index_in_expected_line);
    }
    [[nodiscard]] npi_t get_selected_text_lenght() const noexcept {
        return sc().dcall0(SCI_GETSELTEXT);
    }
    [[nodiscard]] npi_t get_all_text_length() const noexcept {
        return sc().dcall0(SCI_GETTEXTLENGTH);
    }
    [[nodiscard]] npi_t get_lines_count() const noexcept {
        return sc().dcall0(SCI_GETLINECOUNT);
    }
    [[nodiscard]] npi_t get_last_line_length() const noexcept {
        return get_line_lenght(get_lines_count() - 1);
    }

    template<is_container_of_chars T>
    void get_line_text(npi_t line_index, T& buffer) const {
        const npi_t line_length = get_line_lenght(line_index);
        buffer.reserve(line_length + 1);
        dcall2(SCI_GETLINE, line_index,
               reinterpret_cast<sptr_t>(buffer.data()));
    }

    template<is_container_of_chars T>
    void get_all_text(T& buffer) const {
        const npi_t len = get_all_text_length();
        buffer.reserve(len + 1);
        dcall2(SCI_GETTEXT, len + 1, reinterpret_cast<sptr_t>(buffer.data()));
    }

    template<is_container_of_chars T>
    std::pair<npi_t, npi_t> get_selection_text(T& out) const {
        {
            const auto range = selection_op{sc()}.get_selection_range();
            out.reserve(range.second - range.first);
            dcall1_l(SCI_GETSELTEXT, reinterpret_cast<sptr_t>(out.data()));
            return range;
        }
    }
};

struct caret_op: scintilla_ref {
    [[nodiscard]] npi_t get_caret_index() const noexcept {
        return sc().dcall0(SCI_GETCURRENTPOS);
    }
    [[nodiscard]] npi_t get_caret_index_in_line() const noexcept {
        const auto ci = get_caret_index();
        auto to = text_op{sc()};
        return ci - to.get_first_char_index_in_line(to.get_line_index(ci));
    }

    void set_caret_index(const npi_t index) const noexcept {
        sc().dcall1_w(SCI_GOTOPOS, index);
    }
    void caret_to_end() const noexcept {
        set_caret_index(text_op{sc()}.get_all_text_length());
    }
};

struct size_op: scintilla_ref {
    [[nodiscard]] uint8_t get_char_width() const noexcept {
        return static_cast<uint8_t>(sc().dcall2(SCI_TEXTWIDTH, STYLE_DEFAULT,
                                                reinterpret_cast<sptr_t>(" ")));
    }
    // pixels
    [[nodiscard]] uint8_t get_line_height() const noexcept {
        return static_cast<uint8_t>(
            sc().dcall1_w(SCI_TEXTHEIGHT, STYLE_DEFAULT));
    }
    // pixels
    [[nodiscard]] RECT get_window_rect() const noexcept {
        RECT r;
        GetWindowRect(sc().get_native_window(), &r);
        return r;
    }
    [[nodiscard]] uint32_t get_window_width() const noexcept {
        RECT r = get_window_rect();
        return r.right - r.left;
    }
    [[nodiscard]] npi_t get_font_size() const noexcept /*size in pt*/ {
        return static_cast<npi_t>(
            sc().dcall1_w(SCI_STYLEGETSIZE, STYLE_DEFAULT));
    }
    [[nodiscard]] npi_t get_first_visible_line() const noexcept {
        return sc().dcall0(SCI_GETFIRSTVISIBLELINE);
    }
    [[nodiscard]] npi_t get_lines_on_screen() const noexcept {
        return static_cast<npi_t>(sc().dcall0(SCI_LINESONSCREEN));
    }
};

struct look_op: scintilla_ref {
    void set_lexer(Scintilla::ILexer5* lexer) const noexcept {
        sc().dcall2(SCI_SETILEXER, 0, reinterpret_cast<sptr_t>(lexer));
    }
    void set_back_color(int32_t style, COLORREF c) const noexcept {
        PostMessage(sc().get_native_window(), SCI_STYLESETBACK, style, c);
    }
    void force_set_back_color(int32_t style, COLORREF c) const noexcept {
        sc().dcall2(SCI_STYLESETBACK, style, c);
    }
    [[nodiscard]] COLORREF get_background_color(int32_t style) const noexcept {
        return static_cast<COLORREF>(sc().dcall1_w(SCI_STYLEGETBACK, style));
    }
    [[nodiscard]] COLORREF get_text_color(int32_t style) const noexcept {
        return static_cast<COLORREF>(sc().dcall1_w(SCI_STYLEGETFORE, style));
    }
    void clear_all_styles() const noexcept {
        sc().dcall0(SCI_STYLECLEARALL);
    }
    void set_text_color(int32_t style, COLORREF c) const noexcept {
        PostMessage(sc().get_native_window(), SCI_STYLESETFORE, style, c);
    }
    void force_set_text_color(int32_t style, COLORREF c) const noexcept {
        sc().dcall2(SCI_STYLESETFORE, style, c);
    }
    void show_spaces(bool enable) const noexcept {
        int flag = SCWS_INVISIBLE;
        if(enable) {
            flag = SCWS_VISIBLEALWAYS;
        }
        PostMessage(sc().get_native_window(), SCI_SETVIEWWS, flag, 0);
    }

    void show_eol(bool enable) const noexcept {
        PostMessage(sc().get_native_window(), SCI_SETVIEWEOL, enable, 0);
    }
};
struct zoom_op: scintilla_ref {
    int ZOOM_START = -10;
    int ZOOM_END = 50;
    void set_zoom(int zoom) const noexcept {
        sc().dcall1_w(SCI_SETZOOM, std::clamp(zoom, ZOOM_START, ZOOM_END));
    }
    [[nodiscard]] int get_zoom() const noexcept {
        return static_cast<int>(sc().dcall0(SCI_GETZOOM));
    }
};
struct scroll_op: scintilla_ref {
    void scroll(npi_t columns_to_scroll, npi_t lines_to_scroll) const noexcept {
        sc().dcall2(SCI_LINESCROLL, columns_to_scroll, lines_to_scroll);
    }
    [[nodiscard]] npi_t get_horizontal_scroll_offset() const noexcept {
        return sc().dcall0(SCI_GETXOFFSET);
    }
};
} // namespace scintilla

#endif
