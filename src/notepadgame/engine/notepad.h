#pragma once
#include <optional>

#include "boost/signals2.hpp"
#include <Windows.h>
#include <memory>
#include <stdint.h>
#include <string>
#include <thread>
#include "engine/scintilla_wrapper.h"
#include "engine/world.h"
#include "engine/input.h"
#include "engine/time.h"

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

class thread_commands {
public:
    using command_t = std::function<void(notepad*, scintilla*)>;
    using queue_t = std::vector<command_t>;
    friend void swap(queue_t& other, thread_commands& commands) {
        std::lock_guard<std::mutex> lock(commands.mutex_);
        std::swap(other, commands.queue_);
    }
    friend void swap(thread_commands& commands, queue_t& other ) {
        std::lock_guard<std::mutex> lock(commands.mutex_);
        std::swap(commands.queue_,  other);
    }
    void push(command_t key) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push_back(key);
    };
    thread_commands() : queue_(32) {}

private:
    queue_t queue_;
    mutable std::mutex mutex_;
};

class title_line{
    static constexpr auto buf_ = L"robotfindskitten, fps: game_thread {:02} | render_thread {:02}";
public:
   uint64_t game_thread_fps;
   uint64_t render_thread_fps;
    std::wstring make(){
        return std::format(buf_, game_thread_fps, render_thread_fps);
    };
};

// A static Singelton for notepad.exe wrapper
class notepad {
    friend LRESULT CALLBACK hook_wnd_proc(HWND, UINT, WPARAM, LPARAM);
    friend bool hook_GetMessageW(HMODULE);
    friend bool hook_SendMessageW(HMODULE);
    friend bool hook_CreateWindowExW(HMODULE);
    friend bool hook_SetWindowTextW(HMODULE);
    friend BOOL APIENTRY DllMain(const HMODULE h_module,
                                 const DWORD ul_reason_for_call,
                                 LPVOID lp_reserved);

  public:
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
        boost::signals2::signal<void(world&, input::thread_input&,  thread_commands&)>;
    [[nodiscard]] std::optional<std::reference_wrapper<open_signal_t>>
    on_open() {
        return on_open_ ? std::make_optional(std::ref(*on_open_))
                        : std::nullopt;
    }
    title_line window_title{};
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

    void set_window_title(const std::wstring_view title) const {
        SetWindowTextW(main_window_, title.data());
    }

  private:
    static notepad& get() {
        static notepad notepad{};
        return notepad;
    }
    // TODO rewrite commads
    thread_commands commands_;
    thread_commands::queue_t local_commands_;
    void start_game();
    void tick_render();
    explicit notepad();
    opts options_{opts::empty};
    back_buffer buf_;
    HWND main_window_;
    timings::fixed_time_step fixed_time_step_;
    timings::fps_count fps_count_;
    // Live only on startup
    std::unique_ptr<open_signal_t> on_open_;
    LONG_PTR original_proc_; // notepad.exe window proc
    std::optional<scintilla> scintilla_;
};



