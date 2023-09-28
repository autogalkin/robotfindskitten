
#include <boost/lockfree/spsc_queue.hpp>
#include <boost/utility/in_place_factory.hpp>
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
#include <stdint.h>
#include <thread>

#include "engine/buffer.h"
#include "engine/details/base_types.h"
#include "engine/details/nonconstructors.h"
#include "engine/scintilla_wrapper.h"
#include "engine/details/iat_hook.h"

#include "engine/notepad.h"
#include "engine/world.h"
#include "engine/time.h"

extern const int GAME_AREA[2];

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
    : scintilla_(std::nullopt), input_(), main_window_(), original_proc_(0),
      on_open_(std::make_unique<open_signal_t>()), fixed_time_step_(),
      fps_count_(), buf_(GAME_AREA[0], GAME_AREA[1]), commands_() {}

void notepad::tick_render() {
    fps_count_.fps([](auto fps) {
        auto& np = notepad::get();
        auto sub = std::wstring(L"| render_thread ");
        np.window_title.render_thread_fps = fps;
    });
    commands_.consume_all([this](auto f) {
        if (f)
            f(this, &*scintilla_);
    });
    notepad::get().set_window_title(notepad::get().window_title.make());
    const auto pos = scintilla_->get_caret_index();
    buf_.view([this](const std::basic_string<char_size>& buf) {
        scintilla_->set_new_all_text(buf);
    });
    scintilla_->set_caret_index(pos);
}

// game in another thread
void notepad::start_game() {

    static thread_guard game_thread = thread_guard(std::thread(
        [buf = &buf_,
         on_open = std::exchange(
             on_open_, std::unique_ptr<notepad::open_signal_t>{nullptr}),
         &cmds = commands_]() {
            timings::fixed_time_step fixed_time_step;
            world w{buf};
            (*on_open)(w, cmds);
            while (!done.load()) {
                fixed_time_step.sleep();
                w.tick(timings::dt);
            }
        }));

    RECT rect = {NULL};
    GetWindowRect(main_window_, &rect);
    ::SetWindowPos(main_window_, NULL, 0, 0, 0, 0,
                   SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);
    DWORD dwStyle =
        WS_CHILD | WS_VISIBLE | SS_LEFT | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

    GetWindowRect(scintilla_->edit_window_, &rect);
    auto w = CreateWindowEx(WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST,
                            "STATIC", "", dwStyle, 0, 0, rect.right - rect.left,
                            200, main_window_, 0, GetModuleHandle(NULL), 0);
    SetLayeredWindowAttributes(w, popup_window.back_color, 0, LWA_COLORKEY);
    SetWindowPos(w, HWND_TOP, 20, 0, rect.right - rect.left, 200, 0);
    auto deleter = [](HFONT font) { DeleteObject(font); };
    static std::unique_ptr<std::remove_pointer_t<HFONT>, decltype(deleter)>
        hFont{CreateFontW(32, 0, 0, 0, FW_DONTCARE, TRUE, FALSE, FALSE,
                          ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                          DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Arial"),
              std::move(deleter)};
    SendMessage(w, WM_SETFONT, WPARAM(hFont.get()), TRUE);
    ShowWindow(w, SW_SHOW);
    UpdateWindow(w);
    popup_window.window = w;
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
    case WM_DRAWITEM: {
        std::cout << "draw\n";
        break;
    }
    case WM_CTLCOLORSTATIC: {
        auto& np = notepad::get();
        if (np.popup_window.window == (HWND)lp) {
            auto deleter = [](HBRUSH br) { DeleteObject(br); };
            static std::unique_ptr<std::remove_pointer_t<HBRUSH>,
                                   decltype(deleter)>
                hBrushLabel{CreateSolidBrush(np.popup_window.back_color),
                            std::move(deleter)};
            hBrushLabel.reset(CreateSolidBrush(np.popup_window.back_color));
            HDC popup = (HDC)wp;
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
                switch (lpMsg->message) {
                case WM_QUIT:
                    return 0;
                case WM_MOUSEWHEEL: {
                    auto dc = GetDC(np.popup_window.window);
                    SetBkColor(dc, RGB(255, 255, 0));
                    ReleaseDC(np.popup_window.window, dc);

                    // SendMessage(np.popup_window.window, WM_PAINT, 0, 0);
                    auto scroll_delta = GET_WHEEL_DELTA_WPARAM(lpMsg->wParam);
                    // horizontal scroll
                    auto v_scroll = 0;
                    auto h_scroll = 0;
                    if (LOWORD(lpMsg->wParam) & MK_SHIFT) {
                        auto char_width = np.scintilla_->get_char_width();
                        h_scroll =
                            scroll_delta > 0
                                ? -3
                                : (np.scintilla_->get_horizontal_scroll_offset() /
                                                   char_width +
                                               (np.scintilla_
                                                        ->get_window_width() /
                                                    char_width +
                                                3) <
                                           GAME_AREA[1]
                                       ? 3
                                       : 0);
                        // vertical scroll
                    } else if (!(LOWORD(lpMsg->wParam))) {
                        v_scroll =
                            scroll_delta > 0
                                ? -3
                                : ((np.scintilla_->get_lines_on_screen() - 1) <
                                           GAME_AREA[0]
                                       ? 3
                                       : 0);
                    }
                    if (h_scroll || v_scroll) {
                        np.scintilla_->scroll(h_scroll, v_scroll);
                        lpMsg->message = WM_NULL;
                    }
                    break;
                }

                    // Handle keyboard messages
                case WM_KEYDOWN:
                case WM_SYSKEYDOWN: {
                    np.input_.push(static_cast<input::key_t>(lpMsg->wParam));
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
