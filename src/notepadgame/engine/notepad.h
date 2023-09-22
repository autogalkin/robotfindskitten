#pragma once
#include <optional>
#pragma warning(push, 0)
#include "boost/signals2.hpp"
#include <Windows.h>
#include <memory>
#include <thread>
#pragma warning(pop)
#include "engine.h"
#include "world.h"
#include "input.h"
#include "tick.h"

// custom WindowProc
static LRESULT CALLBACK hook_wnd_proc(HWND hwnd, UINT msg, WPARAM wp,
                                      LPARAM lp);
// Capture and block keyboard and mouse inputs
bool hook_GetMessageW(HMODULE module);
// Capture opened files
bool hook_SendMessageW(HMODULE module);
// Capture window handles
bool hook_CreateWindowExW(HMODULE module);
// Block window title updates
bool hook_SetWindowTextW(HMODULE module);

// A static Singelton for notepad.exe wrapper
class notepad {
    friend LRESULT CALLBACK hook_wnd_proc(HWND, UINT, WPARAM, LPARAM);
    friend bool hook_GetMessageW(HMODULE);
    friend bool hook_SendMessageW(HMODULE);
    friend bool hook_CreateWindowExW(HMODULE);
    friend bool hook_SetWindowTextW(HMODULE);

  public:
    static notepad& get() {
        static notepad notepad{};
        return notepad;
    }
    struct opts {
        uint8_t all = static_cast<uint8_t>(values::empty);
        // Not enum class
        enum values : uint8_t {
            // clang-format off
            empty          = 0x0,
            kill_focus     = 0x1 << 0,
            hide_selection = 0x1 << 1,
            disable_mouse  = 0x1 << 2,
            show_eol       = 0x1 << 3,
            show_spaces    = 0x1 << 4
            // clang-format on
        };
        constexpr opts(values v) : all(static_cast<uint8_t>(v)) {}
        constexpr opts(int v) : all(static_cast<uint8_t>(v)) {}
    };

    // TODO channel
    using open_signal_t =
        boost::signals2::signal<void(world&, input::thread_input&)>;
    [[nodiscard]] std::optional<std::reference_wrapper<open_signal_t>>
    on_open() {
        return on_open_ ? std::make_optional(std::ref(*on_open_))
                        : std::nullopt;
    }

    input::thread_input input_state;
    // input get_input_state()
    void connect_to_notepad(const HMODULE module /* notepad.exe module*/,
                            const opts start_options = notepad::opts::empty) {
        options_ = start_options;
        hook_CreateWindowExW(module);
        hook_SendMessageW(module);
        hook_GetMessageW(module);
        hook_SetWindowTextW(module);
    }
    [[nodiscard]] HWND get_window() const { return main_window_; }

    // All games works after initialization this components
    //[[nodiscard]] scintilla& get_engine() { return *scintilla_; }

    void set_window_title(const std::wstring_view title) const {
        SetWindowTextW(main_window_, title.data());
    }

  private:
    back_buffer buf_;
    void start_game();
    explicit notepad();
    ticker render_tick;
    // virtual void tick_frame() override;
    std::optional<scintilla> scintilla_;
    // important, we can't allocate dynamic memory while not connected to
    // notepad.exe
    // std::optional<world> world_;
    // Live only on startup
    std::unique_ptr<open_signal_t> on_open_;
    opts options_{opts::empty};
    HWND main_window_;
    LONG_PTR original_proc_; // notepad.exe window proc
};
