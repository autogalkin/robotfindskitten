/**
 * @file
 * @brief Controls the Notepad.exe
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
 * @brief The Custom Windows Proc
 * We replace the original Notepad.exe Proc with this function
 * to filter windows messages and perform `robotfindskitten` logic:
 *  - Initialize a game after notepad will be ready
 *  - handle WM_SIZE and redraw the all spawned Static Controls
 */
LRESULT CALLBACK hook_wnd_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

/**
 * @brief The Hook for GetMessageW
 * Modify the Notepad.exe application loop: replace GetMessageW, which
 * waits for messages, with PeekMessageW, which polls and continues. This allows
 * running Notepad.exe as a game without waiting for every new message. Also,
 * send NULL to the original loop. Additionally, perform the following: Handle
 * input Block keyboard and mouse inputs for Notepad.exe Scroll Scintilla by the
 * mouse wheel etc."
 */
bool hook_GetMessageW(HMODULE module);

/**
 * @brief The Hook for SendMessageW
 */
bool hook_SendMessageW(HMODULE module);

/**
 * @brief The Hook for CreateWindowExW
 *  - Capture window handles.
 *  - Destroy unnesessary windows: the main menu and the status line
 *  - Create the Scintilla Edit Control
 *  - Replace the original EditControl with the Scintilla
 *  - Start the game after all windows are created
 */
bool hook_CreateWindowExW(HMODULE module);

/**
 * @brief The Hook for SetWindowTextW
 * Prevent the Notepad.exe window title updating
 */
bool hook_SetWindowTextW(HMODULE module);
// NOLINTEND(readability-redundant-declaration)


/**
 * @brief Formats the title line.
 */
struct title_line_args {
    void set_game_thread_fps(timings::fps_count::value_type fps) noexcept {
        game_thread_fps = fps;
    }
    void set_notepad_thread_fps(timings::fps_count::value_type fps) noexcept {
        notepad_thread_fps = fps;
    }
    /**
     * @brief Constructs a title line by
     * formatting the options into a string
     * @return std::wstring for the Win32 API SetWindowTextW
     */
    [[nodiscard]] std::wstring make() {
        return std::format(buf_, game_thread_fps, notepad_thread_fps);
    };

private:
    timings::fps_count::value_type game_thread_fps;
    timings::fps_count::value_type notepad_thread_fps;
    static constexpr auto buf_ = PROJECT_NAME
        " v"
        " " PROJECT_VER L" fps: [ game_thread {:02} ] [ notepad_thread {:02} ]";
};

class notepad;
/**
 * @brief A wrapper for the Win32 API Text Static Control
 * Stores data for rendering a static control, including text, color, and size.
 * The Notepad class uses an array of static control handlers to batch render
 * them together.
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
    operator HWND() { // NOLINT(google-explicit-constructor)
        return wnd_.get();
    }
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
// RAII for joining a thread
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

/**
 * @brief A global singleton loaded into the Notepad.exe process via DLL
 * injection. It serves as the infrastructure connecting the game thread and
 * Notepad.exe.
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
    /*! @brief Some startup options for the notepad */
    struct opts {
        uint8_t all = static_cast<uint8_t>(values::empty);
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
     * @brief Gets signal to connect for an event that fires when Notepad is
     * ready to start a game
     */
    [[nodiscard]] std::optional<std::reference_wrapper<open_signal_t>>
    on_open() noexcept {
        return on_open_ ? std::make_optional(std::ref(*on_open_))
                        : std::nullopt;
    }

    /**
     * @brief The function to call in the dllmain. Setups all hooks.
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

    /*! @brief Gets the Notepad.exe window descriptor */
    [[nodiscard]] HWND get_window() const noexcept {
        return main_window_;
    }
    /*! @brief Sets the Notepad.exe window title */
    void set_window_title(const std::wstring_view title) const noexcept {
        SetWindowTextW(main_window_, title.data());
    }

    /**
     * @brief Adds a render command to the notepad command queue to
     * execute a function with Notepad and Scintilla from the game thread.
     */
    static bool push_command(command_t&& cmd) {
        return notepad::get().commands_.push(cmd);
    };
    /*! @brief Gets the Scintilla dll wrapper */
    [[nodiscard]] scintilla::scintilla_dll& get_scintilla() noexcept {
        return *scintilla_;
    };

    /*! @brief Checks is application windows is WA_INACTIVE in WM_ACTIVATE */
    [[nodiscard]] static bool is_window_active() noexcept {
        return notepad::get().is_active.load();
    }

    /**
     * @brief ShowWindow of the given static control handler.
     */
    void show_static_control(static_control_handler&& ctrl);

    std::vector<static_control_handler>& get_all_static_controls() noexcept {
        return static_controls;
    }
    void
    send_game_fps_to_window_title(timings::fps_count::value_type fps) noexcept {
        window_title_args.set_game_thread_fps(fps);
    }

private:
    explicit notepad() = default;
    static notepad& get() {
        static notepad notepad_{};
        return notepad_;
    }
    void start_game_thread();
    void tick_render();

    // Commands from the other thread
    commands_queue_t commands_{};
    opts options_{opts::empty};
    HWND main_window_{};
    timings::fixed_time_step fixed_time_step_{};
    timings::fps_count fps_count_{};
    // Lives only on startup
    std::unique_ptr<open_signal_t> on_open_{std::make_unique<open_signal_t>()};
    // The Notepad.exe oroginal window proc
    LONG_PTR original_proc_{};
    std::optional<scintilla::scintilla_dll> scintilla_{std::nullopt};
    // The application state
    std::atomic_bool is_active{true};
    title_line_args window_title_args{};
    std::vector<static_control_handler> static_controls;
    // The game
    std::optional<thread_guard> game_thread{std::nullopt};
};

#endif
