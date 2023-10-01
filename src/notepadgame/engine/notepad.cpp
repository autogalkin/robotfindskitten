
#include <boost/lockfree/spsc_queue.hpp>
#include <memory>
#include <optional>

// clang-format off
#include "Richedit.h"
#include <type_traits>
#include <wingdi.h>
#include <winuser.h>
#include <Windows.h>
#include "CommCtrl.h"
// clang-format on
#include <algorithm>
#include <locale>
#include <shared_mutex>
#include <mutex>
#include <cstdint>
#include <thread>

#include "engine/buffer.h"
#include "engine/details/base_types.hpp"
#include "engine/details/nonconstructors.h"
#include "engine/scintilla_wrapper.h"
#include "engine/details/iat_hook.h"

#include "engine/notepad.h"
#include "engine/world.h"
#include "engine/time.h"

extern const pos GAME_AREA;

volatile std::atomic_bool done(false);

class thread_guard: public noncopyable {
    std::thread t;

public:
    explicit thread_guard(std::thread&& t_): t(std::move(t_)) {}
    ~thread_guard() {
        done.store(true);
        if(t.joinable()) {
            t.join();
        }
    }
};

notepad::notepad()
    : scintilla_(std::nullopt), main_window_(), original_proc_(0),
      on_open_(std::make_unique<open_signal_t>()), fixed_time_step_(),
      fps_count_(), buf_(GAME_AREA.y, GAME_AREA.x) {}

// NOLINTBEGIN(bugprone-unchecked-optional-access)
void notepad::tick_render() {
    fps_count_.fps(
        [](auto fps) { notepad::get().window_title.render_thread_fps = fps; });
    // execute all commands from the Game thread
    commands_.consume_all([this](auto f) {
        if(f) {
            f(this, &*scintilla_);
        }
    });
    notepad::get().set_window_title(notepad::get().window_title.make());
    // swap buffers in Scintilla
    // always valid optional
    const auto pos = scintilla_->get_caret_index();
    buf_.view([this](const std::basic_string<char_size>& buf) {
        scintilla_->set_new_all_text(buf);
    });
    scintilla_->set_caret_index(pos);
}

static constexpr int POPUP_WINDOW_HEIGHT = 100;
static constexpr int POPUP_WINDOW_OFFSET = 20;

// game in another thread
void notepad::start_game() {
    static thread_guard game_thread = thread_guard(std::thread(
        [&buf = buf_,
         on_open = std::exchange(
             on_open_, std::unique_ptr<notepad::open_signal_t>{nullptr}),
         &cmds = commands_]() mutable {
            timings::fixed_time_step fixed_time_step;
            timings::fps_count fps_count;
            world w;
            (*on_open)(w, buf, cmds);
            on_open.reset();
            while(!done.load()) {
                fixed_time_step.sleep();
                fps_count.fps([](auto fps) {
                    notepad::push_command([fps](notepad* np, scintilla*) {
                        np->window_title.game_thread_fps = fps;
                    });
                });
                // TODO(Igor): real alpha
                w.tick(timings::dt);
            }
        }));

    RECT rect = {NULL};
    GetWindowRect(main_window_, &rect);
    ::SetWindowPos(main_window_, nullptr, 0, 0, 0, 0,
                   SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);

    DWORD dwStyle =
        WS_CHILD | WS_VISIBLE | SS_LEFT | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

    GetWindowRect(scintilla_->edit_window_, &rect);
    auto* w = CreateWindowEx(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST, "STATIC", "",
        dwStyle, 0, 0, rect.right - rect.left, POPUP_WINDOW_HEIGHT,
        main_window_, nullptr, GetModuleHandle(nullptr), nullptr);
    SetLayeredWindowAttributes(w, popup_window.back_color, 0, LWA_COLORKEY);
    SetWindowPos(w, HWND_TOP, POPUP_WINDOW_OFFSET, 0, rect.right - rect.left,
                 POPUP_WINDOW_HEIGHT, 0);
    static constexpr int font_size = 38;
    static std::unique_ptr<std::remove_pointer_t<HFONT>,
                           decltype(&::DeleteObject)>
        hFont{CreateFontW(font_size, 0, 0, 0, FW_DONTCARE, TRUE, FALSE, FALSE,
                          ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                          DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS,
                          L"Consolas"),
              &::DeleteObject};
    SendMessageW(w, WM_SETFONT, reinterpret_cast<WPARAM>(hFont.get()), TRUE);
    ShowWindow(w, SW_SHOW);
    UpdateWindow(w);
    popup_window.window = w;
}
// NOLINTEND(bugprone-unchecked-optional-access)

constexpr WPARAM MY_START = WM_USER + 100;

LRESULT hook_wnd_proc(HWND hwnd, const UINT msg, const WPARAM wp,
                      const LPARAM lp) {
    auto& np = notepad::get();
    switch(msg) {
    case MY_START: {
        {
            // destroy the status bar
            if(HWND stbar = FindWindowExW(np.get_window(), nullptr,
                                          STATUSCLASSNAMEW, nullptr)) {
                DestroyWindow(stbar);
            }
            // destroy the main menu
            HMENU hMenu = GetMenu(np.get_window());
            SetMenu(notepad::get().get_window(), nullptr);
            DestroyMenu(hMenu);
            RECT rc{};
            GetWindowRect(notepad::get().get_window(), &rc);
            PostMessage(notepad::get().get_window(), WM_SIZE, SIZE_RESTORED,
                        MAKELPARAM(rc.right - rc.left, rc.bottom - rc.top));
        };
        np.start_game();
        break;
    }
    case WM_DESTROY: {
        PostQuitMessage(0);
        return 0;
    }
    case WM_NOTIFY: {
        break;
    }
    case WM_SIZE: {
        if(np.scintilla_) {
            ::SetWindowPos(np.scintilla_->edit_window_, nullptr, 0, 0, 0, 0,
                           SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);
            RECT rect = {NULL};
            GetWindowRect(np.scintilla_->edit_window_, &rect);
            ::SetWindowPos(np.popup_window.window, nullptr, POPUP_WINDOW_OFFSET,
                           0, rect.right - rect.left, POPUP_WINDOW_HEIGHT,
                           SWP_NOACTIVATE);
        }
        break;
    }
    case WM_CTLCOLORSTATIC: {
        auto& np = notepad::get();
        if(np.popup_window.window == reinterpret_cast<HWND>(lp)) {
            static std::unique_ptr<std::remove_pointer_t<HBRUSH>,
                                   decltype(&::DeleteObject)>
                hBrushLabel{CreateSolidBrush(np.popup_window.back_color),
                            &::DeleteObject};
            hBrushLabel.reset(CreateSolidBrush(np.popup_window.back_color));
            HDC popup = reinterpret_cast<HDC>(wp);
            auto& np = notepad::get();
            SetTextColor(popup, np.popup_window.fore_color);
            SetLayeredWindowAttributes(np.popup_window.window,
                                       np.popup_window.back_color, 0,
                                       LWA_COLORKEY);
            SetBkColor(popup, np.popup_window.back_color);
            return reinterpret_cast<LRESULT>(hBrushLabel.get());
        }
        break;
    }
    default:
        break;
    }
    return CallWindowProc(reinterpret_cast<WNDPROC>(np.original_proc_), hwnd,
                          msg, wp, lp);
}
// NOLINTBEGIN(readability-function-cognitive-complexity)
bool hook_GetMessageW(const HMODULE module) {
    static decltype(&GetMessageW) original = nullptr;
    return iat_hook::hook_import<decltype(original)>(
        module, iat_hook::module_name("user32.dll"),
        iat_hook::function_name("GetMessageW"), original,
        [](LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin,
           const UINT wMsgFilterMax) -> BOOL {
            if(HWND stbar = FindWindowExW(notepad::get().get_window(), nullptr,
                                          STATUSCLASSNAMEW, nullptr)) {}
            auto& np = notepad::get();
            while(PeekMessageW(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax,
                               PM_REMOVE)) {
                switch(lpMsg->message) {
                case WM_QUIT:
                    return 0;
                case WM_MOUSEWHEEL: {
                    auto* dc = GetDC(np.popup_window.window);
                    SetBkColor(dc, RGB(255, 255, 0));
                    ReleaseDC(np.popup_window.window, dc);

                    // SendMessage(np.popup_window.window, WM_PAINT, 0, 0);
                    auto scroll_delta = GET_WHEEL_DELTA_WPARAM(lpMsg->wParam);
                    // horizontal scroll
                    if(LOWORD(lpMsg->wParam) & MK_SHIFT) {
                        auto char_width = np.scintilla_->get_char_width();
                        np.scintilla_->scroll(
                            scroll_delta > 0
                                ? -3
                                : (np.scintilla_->get_horizontal_scroll_offset()
                                                   / char_width
                                               + (np.scintilla_
                                                      ->get_window_width()
                                                  / char_width)
                                           < GAME_AREA.x
                                       ? 3
                                       : 0),
                            0);
                        lpMsg->message = WM_NULL;
                        // vertical scroll
                    } else if(!(LOWORD(lpMsg->wParam))) {
                        np.scintilla_->scroll(
                            0, scroll_delta > 0
                                   ? -3
                                   : ((np.scintilla_->get_lines_on_screen() - 1)
                                              < GAME_AREA.y
                                          ? 3
                                          : 0));
                        lpMsg->message = WM_NULL;
                    }
                    break;
                }

                    // Handle keyboard messages
                case WM_KEYDOWN:
                case WM_SYSKEYDOWN: {
                    lpMsg->message = WM_NULL;
                    break;
                }
                    // handle mouse
                case WM_MOUSEMOVE:
                case WM_LBUTTONDOWN:
                case WM_LBUTTONUP:
                case WM_RBUTTONDOWN:
                case WM_RBUTTONUP:
                case WM_LBUTTONDBLCLK:
                case WM_RBUTTONDBLCLK: {
                    if(notepad::opts::disable_mouse & np.options_.all) {
                        lpMsg->message = WM_NULL;
                    }
                    break;
                }

                default:
                    break;
                }
                // work as PeekMessage for the notepad.exe
                TranslateMessage(lpMsg);
                DispatchMessage(lpMsg);
                lpMsg->message =
                    WM_NULL; // send the null to the original dispatch loop
            }

            static std::once_flag once;
            std::call_once(once, [] {
                notepad::get().fixed_time_step_ = timings::fixed_time_step();
            });
            np.fixed_time_step_.sleep();
            np.tick_render();

            return 1;
        });
}

// NOLINTBEGIN(bugprone-easily-swappable-parameters)
bool hook_SendMessageW(const HMODULE module) {
    static decltype(&SendMessageW) original = nullptr;

    return iat_hook::hook_import<decltype(original)>(
        module, iat_hook::module_name("user32.dll"),
        iat_hook::function_name("SendMessageW"), original,
        [](HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) -> LRESULT {
            return original(hWnd, Msg, wParam, lParam);
        });
}

bool hook_CreateWindowExW(HMODULE module) {
    static decltype(&CreateWindowExW) original = nullptr;
    return iat_hook::hook_import<decltype(original)>(
        module, iat_hook::module_name("user32.dll"),
        iat_hook::function_name("CreateWindowExW"), original,

        [](DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName,
           DWORD dwStyle, int X, int Y, int nWidth, int nHeight,
           HWND hWndParent, HMENU hMenu, HINSTANCE hInstance,
           LPVOID lpParam) -> HWND {
            // here creates class 'Notepad' and 'Edit'; can change creation
            // params here
            HWND out_hwnd;

            auto& np = notepad::get();
            if(!lstrcmpW(lpClassName,
                         WC_EDITW)) // handles the edit control creation
                                    // and create custom window
            {
                np.scintilla_.emplace(construct_key<scintilla>{});

                out_hwnd = np.scintilla_->create_native_window(
                    dwExStyle, lpWindowName,
                    dwStyle | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, X, Y, nWidth,
                    nHeight, hWndParent, hMenu, hInstance, lpParam,
                    np.options_.all);

            } else {
                out_hwnd = original(dwExStyle, lpClassName, lpWindowName,
                                    dwStyle | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                                    X, Y, nWidth, nHeight, hWndParent, hMenu,
                                    hInstance, lpParam);
            }
            // catch notepad.exe window
            if(!lstrcmpW(lpClassName, L"Notepad")) {
                np.main_window_ = out_hwnd;
                // patch the wnd proc
                np.original_proc_ =
                    GetWindowLongPtr(np.main_window_, GWLP_WNDPROC);
                SetWindowLongPtr(np.main_window_, GWLP_WNDPROC,
                                 reinterpret_cast<LONG_PTR>(hook_wnd_proc));
                PostMessage(np.main_window_, MY_START, 0, 0);
            }

            return out_hwnd;
        });
}

bool hook_SetWindowTextW(HMODULE module) {
    static decltype(&SetWindowTextW) original = nullptr;
    return iat_hook::hook_import<decltype(original)>(
        module, iat_hook::module_name("user32.dll"),
        iat_hook::function_name("SetWindowTextW"), original,
        [](HWND, LPCWSTR) -> BOOL { return FALSE; });
}
// NOLINTEND(bugprone-easily-swappable-parameters)
// NOLINTEND(readability-function-cognitive-complexity)
