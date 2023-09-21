#pragma once
#include <optional>
#pragma warning(push, 0)
#include "boost/signals2.hpp"
#include <Windows.h>
#include <memory>
#include <thread>
#pragma warning(pop)
#include "engine.h"
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
class notepad_t final : public ticker {
   friend LRESULT CALLBACK hook_wnd_proc(HWND, UINT, WPARAM,LPARAM);
   friend bool hook_GetMessageW(HMODULE);
   friend bool hook_SendMessageW(HMODULE);
   friend bool hook_CreateWindowExW(HMODULE);
   friend bool hook_SetWindowTextW(HMODULE);
public:
    static notepad_t& get() {
        static notepad_t notepad{};
        return notepad;
    }
    struct opts {
        uint8_t all = static_cast<uint8_t>(values::empty);
        // Not enum class
        enum values: uint8_t {
            // clang-format off
            empty          = 0x0,
            kill_focus     = 0x1 << 0,
            hide_selection = 0x1 << 1,
            disable_mouse  = 0x1 << 2,
            show_eol       = 0x1 << 3,
            show_spaces    = 0x1 << 4
            // clang-format on
        };
        constexpr opts(values v): all(static_cast<uint8_t>(v)){}
        constexpr opts(int v): all(static_cast<uint8_t>(v)){}
    };

    using open_signal_t  = boost::signals2::signal<void()>;
    using close_signal_t = boost::signals2::signal<void()>;
    [[nodiscard]] std::optional<std::reference_wrapper<open_signal_t> > on_open() {
        return on_open_ ? std::make_optional(std::ref(*on_open_)) : std::nullopt;
    }

    close_signal_t on_close;
    input_t input;

    void connect_to_notepad(const HMODULE module /* notepad.exe module*/,
                            const opts start_options = notepad_t::opts::empty) {
        options_ = start_options;
        hook_CreateWindowExW(module);
        hook_SendMessageW(module);
        hook_GetMessageW(module);
        hook_SetWindowTextW(module);
    }
    [[nodiscard]] HWND get_window() const { return main_window_; }

    // All games works after initialization this components
    [[nodiscard]] engine_t& get_engine()  {
        return *engine_;
    }

    void set_window_title(const std::wstring_view title) const {
        SetWindowTextW(main_window_, title.data());
    }

private:
    explicit notepad_t()
        : engine_(std::nullopt), input(), main_window_(), original_proc_(0), on_open_(std::make_unique<open_signal_t>()), on_close() {
    }
    virtual void tick_frame() override;
    std::optional<engine_t> engine_;
    // Live only on startup
    std::unique_ptr<open_signal_t> on_open_;
    opts options_{opts::empty};;
    HWND main_window_;
    LONG_PTR original_proc_; // notepad.exe window proc
};
