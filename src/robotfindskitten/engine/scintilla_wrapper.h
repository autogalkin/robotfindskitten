/**
 * @file
 * @brief Scintilla API wrapper
 */

#pragma once


#include <algorithm>
#include <optional>
#include <memory>
#include <string>

#include "Scintilla.h"
#include "ILexer.h"
// clang-format off
#include <Richedit.h>
#include <Windows.h>
#include <CommCtrl.h>
// clang-format on

#include "engine/details/base_types.hpp"
#include "engine/details/nonconstructors.h"

template<typename T>
concept is_container_of_chars = requires(T t) {
    { t.data() } -> std::convertible_to<char*>;
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

class scintilla;

/**
 * @brief Specialization of construct_key for scintilla
 *
 * Allow to create a scintilla in scintilla and in hook_CreateWindowExW
 */
template<>
class construct_key<scintilla> {
    friend bool hook_CreateWindowExW(HMODULE);
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
class scintilla
    : public noncopyable,
      public nonmoveable // Deleted so scintilla objects can not be copied.
{
    friend bool hook_CreateWindowExW(HMODULE);
    friend LRESULT hook_wnd_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

    friend class notepad;

public:

    // Only the hook_CreateWindowExW can create this class
    explicit scintilla(construct_key<scintilla> /*access in friends*/) noexcept
        : native_dll_{LoadLibrary(TEXT("Scintilla.dll")), &::FreeLibrary} {}


    /**
     * @brief Getter for Scintilla HWND window
     *
     * @return HWND of Scintilla Edit Control
     */
    [[nodiscard]] HWND get_native_window() const noexcept {
        return edit_window_;
    }

    /**
     * @brief Set lexer in Scintilla to colorize all document with our custom rules
     *
     * @param lexer Scintilla::ILexer5 child class
     */
    void set_lexer(Scintilla::ILexer5* lexer) const noexcept {
        dcall2(SCI_SETILEXER, 0, reinterpret_cast<sptr_t>(lexer));
    }


    /**
     * @brief Scroll Document
     *
     * @param columns_to_scroll how much characters to scroll +1 : right, -1: left
     * @param lines_to_scroll how much lines to scroll +1 : right, -1: left 
     */
    void scroll(npi_t columns_to_scroll, npi_t lines_to_scroll) const noexcept {
        dcall2(SCI_LINESCROLL, columns_to_scroll, lines_to_scroll);
    }

 // ┌──────────────────────────────────────────────────────────┐
 // │  Caret manipulations                                     │
 // └──────────────────────────────────────────────────────────┘
    /**
     * @brief [TODO:description]
     *
     * @return [TODO:return]
     */
    [[nodiscard]] npi_t get_caret_index() const noexcept {
        return dcall0(SCI_GETCURRENTPOS);
    }

    /**
     * @brief [TODO:description]
     *
     * @return [TODO:return]
     */
    [[nodiscard]] npi_t get_caret_index_in_line() const noexcept;
    void set_caret_index(const npi_t index) const noexcept {
        dcall1(SCI_GOTOPOS, index);
    }
    /**
     * @brief [TODO:description]
     */
    void caret_to_end() const noexcept {
        set_caret_index(get_all_text_length());
    }
 // ┌──────────────────────────────────────────────────────────┐
 // │ Text insertion                                           │
 // └──────────────────────────────────────────────────────────┘
    /**
     * @brief [TODO:description]
     *
     * @param s [TODO:parameter]
     */
    void append_at_caret_position(const std::string& s) const noexcept {
        dcall2(SCI_ADDTEXT, s.size(), reinterpret_cast<sptr_t>(s.data()));
    }

    /**
     * @brief [TODO:description]
     *
     * @param index [TODO:parameter]
     * @param s [TODO:parameter]
     */
    void insert_text(const npi_t index, const std::string& s) const noexcept {
        dcall2(SCI_INSERTTEXT, index, reinterpret_cast<sptr_t>(s.data()));
    }

     /**
      * @brief [TODO:description]
      *
      * @param new_text [TODO:parameter]
      */
     void set_new_all_text(const std::string& new_text) const noexcept {
        dcall1_l(SCI_SETTEXT, reinterpret_cast<sptr_t>(new_text.c_str()));
        dcall0(SCI_EMPTYUNDOBUFFER);
    }   


 // ┌──────────────────────────────────────────────────────────┐
 // │  Selection                                               │
 // └──────────────────────────────────────────────────────────┘


    void set_selection(const npi_t start, const npi_t end) const noexcept {
        dcall2(SCI_SETSEL, start, end);
    }

    void clear_selection(const npi_t caret_pos) const noexcept {
        dcall1(SCI_SETEMPTYSELECTION, caret_pos);
    }

    void select_text_end() const noexcept {
        set_selection(-1, -1);
    }

    [[nodiscard]] std::pair<npi_t, npi_t> get_selection_range() const noexcept {
        return std::make_pair(dcall0(SCI_GETSELECTIONSTART),
                              dcall0(SCI_GETSELECTIONEND));
    }

    void replace_selection(const std::string& new_str) const noexcept {
        dcall1_l(SCI_REPLACESEL, reinterpret_cast<sptr_t>(new_str.c_str()));
    }


 // ┌──────────────────────────────────────────────────────────┐
 // │ Getters for Scintilla attributes                         │
 // └──────────────────────────────────────────────────────────┘
 

    /**
     * @brief [TODO:description]
     *
     * @return [TODO:return]
     */
    [[nodiscard]] npi_t get_horizontal_scroll_offset() const noexcept {
        return dcall0(SCI_GETXOFFSET);
    }

    /**
     * @brief [TODO:description]
     *
     * @param index [TODO:parameter]
     * @return [TODO:return]
     */
    [[nodiscard]] char get_char_at_index(npi_t index) const noexcept {
        return static_cast<char>(dcall1(SCI_GETCHARAT, index));
    }

    /**
     * @brief [TODO:description]
     *
     * @param line_number [TODO:parameter]
     * @return [TODO:return]
     */
    [[nodiscard]] npi_t
    get_first_char_index_in_line(npi_t line_number) const noexcept {
        return dcall1(SCI_POSITIONFROMLINE, line_number);
    }


    [[nodiscard]] npi_t
    get_last_char_index_in_line(npi_t line_number) const noexcept {
        return dcall1(SCI_GETLINEENDPOSITION, line_number);
    }


    [[nodiscard]] npi_t get_line_lenght(npi_t line_index) const noexcept {
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
    [[nodiscard]] RECT get_window_rect() const noexcept {
        RECT r;
        GetWindowRect(get_native_window(), &r);
        return r;
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

    template<is_container_of_chars T>
    void get_line_text(npi_t line_index, T& buffer) const noexcept;
    template<is_container_of_chars T>
    void get_all_text(T& buffer) const noexcept;
    template<is_container_of_chars T>
    std::pair<npi_t, npi_t> get_selection_text(T& out) const noexcept;

    // look dev
    void set_back_color(int32_t style,
                        COLORREF c) const noexcept; // post message
    void force_set_back_color(int32_t style,
                              COLORREF c) const noexcept; // send message
    [[nodiscard]] COLORREF get_background_color(int32_t style) const noexcept {
        return static_cast<COLORREF>(dcall1(SCI_STYLEGETBACK, style));
    }
    [[nodiscard]] COLORREF get_text_color(int32_t style) const noexcept {
        return static_cast<COLORREF>(dcall1(SCI_STYLEGETFORE, style));
    }
    void clear_all_styles() const noexcept {
        dcall0(SCI_STYLECLEARALL);
    }
    void set_text_color(int32_t style,
                        COLORREF c) const noexcept; // post message
    void force_set_text_color(int32_t style,
                              COLORREF c) const noexcept; // send message
    void show_spaces(bool enable) const noexcept;
    void show_eol(bool enable) const noexcept {
        PostMessage(get_native_window(), SCI_SETVIEWEOL, enable, 0);
    }
    void set_zoom(int zoom) const noexcept {
        static constexpr int ZOOM_START = -10;
        static constexpr int ZOOM_END = 50;
        dcall1(SCI_SETZOOM, std::clamp(zoom, ZOOM_START, ZOOM_END));
    }
    [[nodiscard]] int get_zoom() const noexcept {
        return static_cast<int>(dcall0(SCI_GETZOOM));
    }

    ~scintilla();

    HWND create_native_window(DWORD dwExStyle, LPCWSTR lpWindowName,
                              DWORD dwStyle, int X, int Y, int nWidth,
                              int nHeight, HWND hWndParent, HMENU hMenu,
                              HINSTANCE hInstance, LPVOID lpParam,
                              uint8_t start_options);
    void init_direct_access_to_scintilla();

private:
    using direct_function_t = npi_t (*)(sptr_t, int, uptr_t, sptr_t);
    // NOLINTBEGIN(modernize-use-nodiscard)
    npi_t dcall0(int message) const {
        return direct_function_(direct_wnd_ptr_, message, 0, 0);
    }
    npi_t dcall1(int message, uptr_t w) const {
        return direct_function_(direct_wnd_ptr_, message, w, 0);
    }
    npi_t dcall1_l(int message, sptr_t l) const {
        return direct_function_(direct_wnd_ptr_, message, 0, l);
    }
    npi_t dcall2(int message, uptr_t w, sptr_t l) const {
        return direct_function_(direct_wnd_ptr_, message, w, l);
    }
    // NOLINTEND(modernize-use-nodiscard)
    std::unique_ptr<std::remove_pointer_t<HMODULE>, decltype(&::FreeLibrary)>
        native_dll_;
    HWND edit_window_{nullptr};
    sptr_t direct_wnd_ptr_{0};
    npi_t (*direct_function_)(sptr_t, int, uptr_t, sptr_t){nullptr};
};
