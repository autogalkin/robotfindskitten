#include "buffer.h"
#include "details/nonconstructors.h"
#include "tick.h"
#include <boost/utility/in_place_factory.hpp>
#include <optional>
#pragma warning(push, 0)
// clang-format off
#include "Richedit.h"
#include <Windows.h>
#include "CommCtrl.h"
// clang-format on
#include <algorithm>
#include <locale>
#include <shared_mutex>
#include <mutex>
#include <thread>
#pragma warning(pop)

#include "details/gamelog.h"
#include "engine.h"
#include "iat_hook.h"
#include "input.h"
#include "notepad.h"
#include "world.h"

// TODO to inside buf
static volatile std::atomic_bool done(false);

class thread_guard : public noncopyable {
    std::thread t;
  public:
    explicit thread_guard(std::thread&& t_) : t(std::move(t_)) {}
    ~thread_guard() {
        done.store(true);
        if (t.joinable()) {
            t.join();
        }
    }
};


notepad::notepad()
    : scintilla_(std::nullopt), input_state(), main_window_(),
      original_proc_(0), on_open_(std::make_unique<open_signal_t>()),
       render_tick(), buf_(150, 30) {}

void notepad::start_game() {
    // render in main thread
    render_tick.on_tick.connect([this](gametime::duration d) {
        const auto pos = scintilla_->get_caret_index();
        {
            buf_.view([this](const std::basic_string<char_size>& buf) {
                scintilla_->set_new_all_text(buf);
            });
        }
        scintilla_->set_caret_index(pos);
        set_window_title(L"notepadgame fps: " +
                         std::to_wstring(render_tick.get_current_frame_rate()));
    });

    // game in another thread
    static thread_guard game_thread = thread_guard(std::thread(
        [buf = &buf_,
         on_open = std::exchange(
             on_open_, std::unique_ptr<notepad::open_signal_t>{nullptr}),
         &input = input_state]() {
            ticker game_ticker;
            world w{buf, game_ticker.on_tick};
            (*on_open)(w, input);
            game_ticker.reset_to_start();
            while (!done.load()) {
                game_ticker.tick_frame();
            }
        }));
}

LRESULT hook_wnd_proc(HWND hwnd, const UINT msg, const WPARAM wp,
                      const LPARAM lp) {
    auto& np = notepad::get();
    switch (msg) {
    case WM_USER + 100: // on initialize
    {
        {
            // destroy the status bar
            if (const HWND stbar = FindWindowExW(np.get_window(), nullptr,
                                                 STATUSCLASSNAMEW, nullptr)) {
                DestroyWindow(stbar);
            }
            // destroy the main menu
            const HMENU hMenu = GetMenu(np.get_window());
            SetMenu(notepad::get().get_window(), nullptr);
            DestroyMenu(hMenu);
            RECT rc{};
            GetClientRect(notepad::get().get_window(), &rc);
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

        // scintilla notifications
        if (const auto* n = reinterpret_cast<SCNotification*>(lp)) {
            switch (n->updated) {
            case SC_UPDATE_H_SCROLL: {
                // auto &e = np.get_engine();
                // e.set_h_scroll(e.get_horizontal_scroll_offset() /
                //                 e.get_char_width());
                break;
            }
            case SC_UPDATE_V_SCROLL: {
                // auto &e = np.get_engine();
                // e.set_v_scroll(e.get_first_visible_line());
                break;
            }
            default:
                break;
            }
        }

        break;
    }
    case WM_SIZING:
    case WM_SIZE: {
        const UINT width = LOWORD(lp);
        const UINT height = HIWORD(lp);
        if (np.scintilla_) {
        }
        // np.scintilla_->get_on_resize()(width, height);
        break;
    }
    default:
        break;
    }
    return CallWindowProc(reinterpret_cast<WNDPROC>(np.original_proc_), hwnd,
                          msg, wp, lp);
}

bool hook_GetMessageW(const HMODULE module) {
    static decltype(&GetMessageW) original = nullptr;
    return iat_hook::hook_import<decltype(original)>(
        module, "user32.dll", "GetMessageW", original,
        [](const LPMSG lpMsg, const HWND hWnd, const UINT wMsgFilterMin,
           const UINT wMsgFilterMax) -> BOOL {
            if (const HWND stbar =
                    FindWindowExW(notepad::get().get_window(), nullptr,
                                  STATUSCLASSNAMEW, nullptr)) {
            }
            auto& np = notepad::get();
            // TODO описание как из getmessage в peek message
            while (PeekMessageW(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax,
                                PM_REMOVE)) {
                // gamelog::cout(std::hex, "get message", lpMsg->message);
                switch (lpMsg->message) {
                case WM_QUIT:
                    return 0;

                    // Handle keyboard messages
                case WM_KEYDOWN:
                case WM_SYSKEYDOWN: {
                    np.input_state.push(
                        static_cast<input::key_t>(lpMsg->wParam));
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
                    if (notepad::opts::disable_mouse & np.options_.all)
                        lpMsg->message = WM_NULL;
                    break;
                }
                default:
                    break;
                }

                TranslateMessage(lpMsg);
                DispatchMessage(lpMsg);
                lpMsg->message =
                    WM_NULL; // send the null to the original dispatch loop
            }

            static std::once_flag once;
            std::call_once(once,
                           [] { notepad::get().render_tick.reset_to_start(); });
            notepad::get().render_tick.tick_frame();

            return 1;
        });
}

bool hook_SendMessageW(const HMODULE module) {
    static decltype(&SendMessageW) original = nullptr;

    return iat_hook::hook_import<decltype(original)>(
        module, "user32.dll", "SendMessageW", original,

        [](HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) -> LRESULT {
            return original(hWnd, Msg, wParam, lParam);
        });
}

bool hook_CreateWindowExW(HMODULE module) {
    static decltype(&CreateWindowExW) original = nullptr;
    return iat_hook::hook_import<decltype(original)>(
        module, "user32.dll", "CreateWindowExW", original,

        [](DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName,
           DWORD dwStyle, int X, int Y, int nWidth, int nHeight,
           HWND hWndParent, HMENU hMenu, HINSTANCE hInstance,
           LPVOID lpParam) -> HWND {
            // here creates class 'Notepad' and 'Edit'; can change creation
            // params here
            HWND out_hwnd;

            auto& np = notepad::get();
            if (!lstrcmpW(lpClassName,
                          WC_EDITW)) // handles the edit control creation and
                                     // create custom window
            {
                np.scintilla_.emplace(construct_key<scintilla>{});

                out_hwnd = np.scintilla_->create_native_window(
                    dwExStyle, lpWindowName, dwStyle, X, Y, nWidth, nHeight,
                    hWndParent, hMenu, hInstance, lpParam, np.options_.all);

            } else {
                out_hwnd = original(dwExStyle, lpClassName, lpWindowName,
                                    dwStyle, X, Y, nWidth, nHeight, hWndParent,
                                    hMenu, hInstance, lpParam);
            }
            // catch notepad.exe window
            if (!lstrcmpW(lpClassName, L"Notepad")) {

                np.main_window_ = out_hwnd;
                // patch the wnd proc
                np.original_proc_ =
                    GetWindowLongPtr(np.main_window_, GWLP_WNDPROC);
                SetWindowLongPtr(np.main_window_, GWLP_WNDPROC,
                                 reinterpret_cast<LONG_PTR>(hook_wnd_proc));
                PostMessage(np.main_window_, WM_USER + 100, 0, 0);
            }

            return out_hwnd;
        });
}

bool hook_SetWindowTextW(HMODULE module) {
    static decltype(&SetWindowTextW) original = nullptr;
    return iat_hook::hook_import<decltype(original)>(
        module, "user32.dll", "SetWindowTextW", original,
        [](HWND hWnd, LPCWSTR lpString) -> BOOL { return FALSE; });
}
