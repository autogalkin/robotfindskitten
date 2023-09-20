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
#include <mutex>
#pragma warning(pop)

#include "details/gamelog.h"
#include "engine.h"
#include "iat_hook.h"
#include "input.h"
#include "notepad.h"
#include "world.h"



LRESULT hook_wnd_proc(HWND hwnd, const UINT msg, const WPARAM wp,
                                 const LPARAM lp) {
    auto& np = notepad_t::get();
    switch (msg) {
    case WM_USER + 100: // on initialize
  {
    {
      // destroy the status bar
      if (const HWND stbar =
              FindWindowExW(np.get_window(), nullptr,
                            STATUSCLASSNAMEW, nullptr)) {
        DestroyWindow(stbar);
      }
      // destroy the main menu
      const HMENU hMenu = GetMenu(np.get_window());
      SetMenu(notepad_t::get().get_window(), nullptr);
      DestroyMenu(hMenu);
      RECT rc{};
      GetClientRect(notepad_t::get().get_window(), &rc);
      PostMessage(notepad_t::get().get_window(), WM_SIZE, SIZE_RESTORED,
                  MAKELPARAM(rc.right - rc.left, rc.bottom - rc.top));
    }
    ;
    np.on_open_->connect([] { gamelog::cout("Notepad Connected and Initialized"); });
    np.on_open_->operator()();
    np.on_open_.reset();
    break;
  }
  case WM_DESTROY: {
    np.on_close();
    PostQuitMessage(0);
    return 0;
  }
  case WM_NOTIFY: {

    // scintilla notifications
    if (const auto *n = reinterpret_cast<SCNotification *>(lp)) {
      switch (n->updated) {
      case SC_UPDATE_H_SCROLL: {
        auto &e = np.get_engine();
        //e.set_h_scroll(e.get_horizontal_scroll_offset() /
        //                e.get_char_width());
        break;
      }
      case SC_UPDATE_V_SCROLL: {
        //auto &e = np.get_engine();
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
    if (np.engine_)
      np.engine_->get_on_resize()(width, height);
    break;
  }
  default:
    break;
  }
  return CallWindowProc(reinterpret_cast<WNDPROC>(np.original_proc_), hwnd,
                        msg, wp, lp);
}

bool hook_GetMessageW(const HMODULE module)  {
  static decltype(&GetMessageW) original = nullptr;
  return iat_hook::hook_import<decltype(original)>(
      module, "user32.dll", "GetMessageW", original,
      [](const LPMSG lpMsg, const HWND hWnd, const UINT wMsgFilterMin,
         const UINT wMsgFilterMax) -> BOOL {
        if (const HWND stbar =
                FindWindowExW(notepad_t::get().get_window(), nullptr,
                              STATUSCLASSNAMEW, nullptr)) {
        }
        auto& np= notepad_t::get();
        // TODO описание как из getmessage в peek message
        while (PeekMessageW(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax,
                            PM_REMOVE)) {
          // gamelog::cout(std::hex, "get message", lpMsg->message);
          switch (lpMsg->message) {
          case WM_QUIT:
            return 0;

            // Handle keyboard messages
          case WM_KEYDOWN:
          case WM_SYSKEYDOWN:{
              np.input.push(static_cast<input_state_t::key_t>(lpMsg->wParam));
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
            if (notepad_t::opts::disable_mouse & np.options_.all)
              lpMsg->message = WM_NULL;
            break;
          }
          default:
            break;
          }

          TranslateMessage(lpMsg);
          DispatchMessage(lpMsg);
          lpMsg->message = WM_NULL; // send the null to original dispatch loop
        }

        try{
            static std::once_flag once;
            std::call_once(once, [] { notepad_t::get().reset_to_start(); });

            notepad_t::get().tick_frame();

        }
         catch([[maybe_unused]] std::exception& e){
             gamelog::cout(e.what());
            PostQuitMessage(0);
        }

        return 1;
      });
}

bool hook_SendMessageW(const HMODULE module){
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
         DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent,
         HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam) -> HWND {
        // here creates class 'Notepad' and 'Edit'; can change creation params
        // here
        HWND out_hwnd;

        auto& np = notepad_t::get();
        if (!lstrcmpW(lpClassName,
                      WC_EDITW)) // handles the edit control creation and create
                                 // custom window
        {
          np.engine_.emplace(construct_key<engine_t>{});

          out_hwnd = np.engine_->create_native_window(
              dwExStyle, lpWindowName, dwStyle, X, Y, nWidth, nHeight,
              hWndParent, hMenu, hInstance, lpParam, np.options_.all);

        } else {
          out_hwnd =
              original(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y,
                       nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
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

bool hook_SetWindowTextW(HMODULE module){
  static decltype(&SetWindowTextW) original = nullptr;
  return iat_hook::hook_import<decltype(original)>(
      module, "user32.dll", "SetWindowTextW", original,
      [](HWND hWnd, LPCWSTR lpString) -> BOOL { return FALSE; });
}

void notepad_t::tick_frame() {
  engine_->get_world().backbuffer.send();
  ticker::tick_frame();
  set_window_title(L"notepadgame fps: " +
                   std::to_wstring(get_current_frame_rate()));
  input.clear();
}
