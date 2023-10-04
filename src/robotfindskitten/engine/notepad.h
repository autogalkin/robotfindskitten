#pragma once
#include <optional>
#include <memory>
#include <cstdint>
#include <string_view>

#include <Windows.h>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include "boost/signals2.hpp"
#include <boost/lockfree/spsc_queue.hpp>

#include "engine/scintilla_wrapper.h"
#include "engine/time.h"
#include "config.h"

// custom WindowProc
// NOLINTBEGIN(readability-redundant-declaration)
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
// NOLINTEND(readability-redundant-declaration)

struct title_line {
    uint32_t game_thread_fps;
    uint32_t render_thread_fps;
    [[nodiscard]] std::wstring make() {
        return std::format(buf_, game_thread_fps, render_thread_fps);
    };

private:
    static constexpr auto buf_ =
        PROJECT_NAME L", fps: game_thread {:02} | render_thread {:02}";
};

struct static_control {
    using id_t = boost::uuids::uuid;
    // std::shared_ptr because boost::lockfree::spsc_queue no supported emplace
    // See '#31 Support moveable objects and allow emplacing (Pull request)'
    // https://github.com/boostorg/lockfree/pull/31
    using window_t = std::shared_ptr<std::remove_pointer_t<HWND>>;

private:
    window_t wnd_ = {nullptr};
    id_t id_      = make_id();

public:
    static id_t make_id() noexcept {
        return boost::uuids::random_generator()();
    }
    static_control& with_position(pos where) noexcept {
        this->position = where;
        return *this;
    }
    static_control& with_size(pos size) noexcept {
        this->size = size;
        return *this;
    }
    static_control& with_text(std::string_view text) noexcept {
        this->text = text;
        return *this;
    }
    static_control& with_text_color(COLORREF color) noexcept {
        this->fore_color = color;
        return *this;
    }
    void show(notepad* np)noexcept;

    [[nodiscard]] id_t get_id() const noexcept {
        return id_;
    }
    operator HWND() { // NOLINT(google-explicit-constructor)
        return wnd_.get();
    }
    [[nodiscard]] HWND get_wnd() const noexcept {
        return wnd_.get();
    }
    pos size;
    pos position;
    std::string_view text;
    COLORREF fore_color = RGB(0, 0, 0);
};

static_control
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
show_static_control(HWND parent_window, COLORREF back_color_alpha,
                    COLORREF fore_color, pos size, pos position);

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
        constexpr opts(values v) // NOLINT(google-explicit-constructor)
            : all(static_cast<uint8_t>(v)) {}
        constexpr opts(int v) // NOLINT(google-explicit-constructor)
            : all(static_cast<uint8_t>(v)) {}
    };

    using command_t = std::function<void(notepad*, scintilla*)>;
    static constexpr int command_queue_capacity = 32;
    using commands_queue_t = boost::lockfree::spsc_queue<
        command_t, boost::lockfree::capacity<command_queue_capacity>>;
    using open_signal_t = boost::signals2::signal<void(
        std::shared_ptr<std::atomic_bool> shutdown_signal)>;
    [[nodiscard]] std::optional<std::reference_wrapper<open_signal_t>>
    on_open() {
        return on_open_ ? std::make_optional(std::ref(*on_open_))
                        : std::nullopt;
    }
    title_line window_title{};

    void connect_to_notepad(const HMODULE module /* notepad.exe module*/,
                            const opts start_options = notepad::opts::empty) {
        options_ = start_options;
        hook_CreateWindowExW(module);
        hook_SendMessageW(module);
        hook_GetMessageW(module);
        hook_SetWindowTextW(module);
    }
    [[nodiscard]] HWND get_window() const {
        return main_window_;
    }

    void set_window_title(const std::wstring_view title) const {
        SetWindowTextW(main_window_, title.data());
    }
    // TODO(Igor): maybe a ecs processor which will push all commands together?
    static bool push_command(command_t&& cmd) {
        return notepad::get().commands_.push(cmd);
    };

    std::vector<static_control> static_controls;
    COLORREF back_color = RGB(255, 255, 255);


private:
    static notepad& get() {
        static notepad notepad{};
        return notepad;
    }

    commands_queue_t commands_;

    void start_game();
    void tick_render();
    explicit notepad();
    opts options_{opts::empty};
    HWND main_window_;
    timings::fixed_time_step fixed_time_step_;
    timings::fps_count fps_count_;
    // Live only on startup
    std::unique_ptr<open_signal_t> on_open_;
    LONG_PTR original_proc_; // notepad.exe window proc
    std::optional<scintilla> scintilla_;
};
