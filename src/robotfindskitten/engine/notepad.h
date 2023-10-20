/**
 * @file
 * @brief Control Notepad.exe
 *
 */

#ifndef _CPP_PROJECTS_ROBOTFINDSKITTEN_SRC_ROBOTFINDSKITTEN_ENGINE_NOTEPAD_H
#define _CPP_PROJECTS_ROBOTFINDSKITTEN_SRC_ROBOTFINDSKITTEN_ENGINE_NOTEPAD_H

#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>
#include <atomic>

#include <boost/signals2.hpp>
#include <Windows.h>
#include <boost/lockfree/spsc_queue.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>

#include "config.h"
#include "engine/scintilla_wrapper.h"
#include "engine/time.h"

// NOLINTBEGIN(readability-redundant-declaration)

/**
 * @brief Custom Windows Proc
 *
 * Replace original Notepad.exe proc with this function
 * and filter windows messages and perform robotfindskitten logic:
 *  - Initialize a game after notepad will be ready
 *  - handle WM_SIZE and redraw all spawned Static Controls
 */
LRESULT CALLBACK hook_wnd_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

/**
 * @brief A Hook for GetMessageW
 *
 * Modify Notepad.exe application loop: replace GetMessageW, which
 * waits messages with PeekMessageW which just poll and continue. it allows
 * run Notepad.exe as a game without waits every new message. And
 * send NULL to the original loop
 * Also performs:
 *  - Handle input
 *  - Block keyboard and mouse inputs for Notepad.exe
 *  - Scroll Skintilla on mouse wheel
 *  - Tick a render loop
 */
bool hook_GetMessageW(HMODULE module);

/**
 * @brief Hook for SendMessageW
 */
bool hook_SendMessageW(HMODULE module);

/**
 * @brief Hook for CreateWindowExW
 *
 *  - Capture window handles.
 *  - Destroy unnesessary windows: main menu and status line
 *  - Create Scintilla Edit Control
 *  - Replace original EditControl with Scintilla
 *  - Start a game after all windows are created
 */
bool hook_CreateWindowExW(HMODULE module);

/**
 * @brief Hook for SetWindowTextW
 *
 * Prevent Notepad.exe window title updating
 *
 */
bool hook_SetWindowTextW(HMODULE module);

// NOLINTEND(readability-redundant-declaration)

/**
 * @class title_line
 * @brief Manage title line with changing values
 */
struct title_line_args {
    void set_game_thread_fps(timings::fps_count::value_type fps) noexcept {
        game_thread_fps = fps;
    }
    void set_notepad_thread_fps(timings::fps_count::value_type fps) noexcept {
        notepad_thread_fps = fps;
    }
    /**
     * @brief Construct a title line
     *
     * Format options into string
     *
     * @return std::wstring for Win32 API SetWindowTextW
     */
    [[nodiscard]] std::wstring make() {
        return std::format(buf_, game_thread_fps, notepad_thread_fps);
    };

private:
    timings::fps_count::value_type game_thread_fps;
    timings::fps_count::value_type notepad_thread_fps;
    static constexpr auto buf_ = PROJECT_NAME
        " v " PROJECT_VER L" fps: [ game_thread {:02} ] [ notepad_thread {:02} ]";
};

class notepad;
/**
 * @class static_control
 * @brief Wrapper for Win32 API Text Static Control
 *
 * Hold some data for render a static control: text, color, size..
 * class notepad use array of static_control to batch render all together
 *
 */
struct static_control_handler {
    friend notepad;
    using id_t = boost::uuids::uuid;
    // std::shared_ptr because boost::lockfree::spsc_queue no supported emplace
    // See '#31 Support moveable objects and allow emplacing (Pull request)'
    // https://github.com/boostorg/lockfree/pull/31
    using window_t = std::shared_ptr<std::remove_pointer_t<HWND>>;

private:
    static id_t make_id() noexcept {
        return boost::uuids::random_generator()();
    }
    window_t wnd_ = {nullptr};
    id_t id_ = make_id();
    pos size_;
    pos position_;
    std::string text_;
    COLORREF fore_color_ = RGB(0, 0, 0);

public:
    static_control_handler& with_position(pos where) noexcept {
        position_ = where;
        return *this;
    }
    static_control_handler& with_size(pos size) noexcept {
        size_ = size;
        return *this;
    }
    [[nodiscard]] pos get_size() const noexcept {
        return size_;
    }
    static_control_handler& with_text(std::string_view text) noexcept {
        text_ = text;
        return *this;
    }
    [[nodiscard]] std::string_view get_text() const noexcept {
        return text_;
    }
    static_control_handler& with_fore_color(COLORREF color) noexcept {
        fore_color_ = color;
        return *this;
    }
    operator HWND() { // NOLINT(google-explicit-constructor)
        return wnd_.get();
    }
    [[nodiscard]] id_t get_id() const noexcept {
        return id_;
    }
    [[nodiscard]] HWND get_wnd() const noexcept {
        return wnd_.get();
    }
    [[nodiscard]] pos get_pos() const noexcept {
        return position_;
    }
    [[nodiscard]] COLORREF get_fore_color() const noexcept {
        return fore_color_;
    }
};

class thread_guard {
    std::thread t_;
    std::shared_ptr<std::atomic_bool> shutdown_token_ =
        std::make_shared<std::atomic_bool>(false);

public:
    explicit thread_guard(std::thread t,
                          std::shared_ptr<std::atomic_bool> shutdown)
        : t_(std::move(t)), shutdown_token_(std::move(shutdown)) {}
    thread_guard(const thread_guard&) = delete;
    thread_guard& operator=(const thread_guard&) = delete;
    thread_guard(thread_guard&&) = default;
    thread_guard& operator=(thread_guard&&) = default;
    ~thread_guard() {
        if(shutdown_token_) {
            shutdown_token_->store(true);
        }
        if(t_.joinable()) {
            try {
                t_.join();
            } catch(const std::system_error&) {
                /* suppressed */
            }
        }
    }
};

// A static Singelton for notepad.exe wrapper
/**
 * @class notepad
 * @brief A global Singelton that has been load into Notepad.exe by DLL
 * injection
 *
 * Provides all infrastructure between the game and Notepad.exe
 *
 */
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
    /**
     * @class opts
     * @brief Some startup options for notepad
     *
     */
    struct opts {
        uint8_t all = static_cast<uint8_t>(values::empty);
        enum values : uint8_t { // Not enum class
            // clang-format off
            empty          = 0x0,
            kill_focus     = 0x1 << 0,
            hide_selection = 0x1 << 1,
            disable_mouse  = 0x1 << 2,
            show_eol       = 0x1 << 3,
            show_spaces    = 0x1 << 4
            // clang-format on
        };
        constexpr opts(values v) // NOLINT(google-explicit-constructor)
            : all(static_cast<uint8_t>(v)) {}
        constexpr opts(int v) // NOLINT(google-explicit-constructor)
            : all(static_cast<uint8_t>(v)) {}
    };

    using command_t = std::function<void(notepad&, scintilla::scintilla_dll&)>;
    static constexpr int command_queue_capacity = 32;
    using commands_queue_t = boost::lockfree::spsc_queue<
        command_t, boost::lockfree::capacity<command_queue_capacity>>;
    using open_signal_t = boost::signals2::signal<void(
        std::shared_ptr<std::atomic_bool> shutdown_signal)>;

    /**
     * @brief Get signal to connect for event that fired on, when all notepad
     * ready for start a game
     *
     * @return a signal
     */
    [[nodiscard]] std::optional<std::reference_wrapper<open_signal_t>>
    on_open() noexcept {
        return on_open_ ? std::make_optional(std::ref(*on_open_))
                        : std::nullopt;
    }

    /**
     * @brief Initial function to call on start, setup all hooks,
     * Must call only once
     *
     * @param module Notepad.exe application module
     * @param start_options startup options
     */
    void connect_to_notepad(
        const HMODULE module /* notepad.exe module*/,
        const opts start_options = notepad::opts::empty) noexcept {
        options_ = start_options;
        hook_CreateWindowExW(module);
        hook_SendMessageW(module);
        hook_GetMessageW(module);
        hook_SetWindowTextW(module);
    }

    /**
     * @brief Get Notepad.exe Main Window descriptor
     *
     * @return HWND descriptor
     */
    [[nodiscard]] HWND get_window() const noexcept {
        return main_window_;
    }

    /**
     * @brief Set Notepad.exe window title
     *
     * @param title text to set
     */
    void set_window_title(const std::wstring_view title) const noexcept {
        SetWindowTextW(main_window_, title.data());
    }

    /**
     * @brief Add a render command to notepad command queue
     *
     * Execute a function with Notepad and Scintilla from the game thread
     *
     * @param cmd function to execute in the notepad thread
     * @return Success
     */
    static bool push_command(command_t&& cmd) {
        return notepad::get().commands_.push(cmd);
    };
    /**
     * @brief Get Scintilla Wrapper
     *
     * @return scintilla wrapper class
     */
    [[nodiscard]] scintilla::scintilla_dll& get_scintilla() noexcept {
        return *scintilla_;
    };

    [[nodiscard]] static bool is_window_active() noexcept {
        return notepad::get().is_active.load();
    }

    /**
     * @brief [TODO:description]
     *
     * @param np [TODO:parameter]
     * @return  reference to self
     */
    void show_static_control(static_control_handler&& ctrl);

    void send_game_fps(timings::fps_count::value_type fps) noexcept {
        window_title_args.set_game_thread_fps(fps);
    }

    std::vector<static_control_handler>& get_all_static_controls() noexcept {
        return static_controls;
    }

private:
    explicit notepad() = default;
    static notepad& get() {
        static notepad notepad_{};
        return notepad_;
    }
    void start_game_thread();
    void tick_render();

    // commands from the other thread
    commands_queue_t commands_{};
    opts options_{opts::empty};
    HWND main_window_{};
    timings::fixed_time_step fixed_time_step_{};
    timings::fps_count fps_count_{};
    // Live only on startup
    std::unique_ptr<open_signal_t> on_open_{std::make_unique<open_signal_t>()};
    // Notepad.exe window proc
    LONG_PTR original_proc_{};
    std::optional<scintilla::scintilla_dll> scintilla_{std::nullopt};
    // Indicate the application state
    std::atomic_bool is_active{true};
    title_line_args window_title_args{};
    std::vector<static_control_handler> static_controls;
    std::optional<thread_guard> game_thread{std::nullopt};
};

#endif
