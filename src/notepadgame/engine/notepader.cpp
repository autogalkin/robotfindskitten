
#pragma warning(push, 0)
#include <algorithm>
#include <locale>
#include <mutex>
#include "Richedit.h"
#include "CommCtrl.h"
#include <Windows.h>
#pragma warning(pop)

#include "details/gamelog.h"
#include "notepader.h"
#include "iat_hook.h"
#include "engine.h"
#include "input.h"
#include "world.h"








void notepader::close()
{
	on_close_();
	PostQuitMessage(0);
}

LRESULT notepader::hook_wnd_proc(HWND hwnd, const UINT msg, const WPARAM wp, const LPARAM lp)
{
	//gamelog::cout(std::hex, "wnd proc", msg);
	switch (msg)
	{
	case WM_USER + 100: // on initialize
	{
			{
				// destroy the status bar
				if(const HWND stbar = FindWindowExW(notepader::get().get_main_window(), nullptr,  STATUSCLASSNAMEW, nullptr)){
					DestroyWindow(stbar);
				}
				// destroy the main menu
				const HMENU hMenu = GetMenu(notepader::get().get_main_window());
				SetMenu(notepader::get().get_main_window(), nullptr);
				DestroyMenu(hMenu);
				RECT rc{};
				GetClientRect(notepader::get().get_main_window(), &rc);
				PostMessage(notepader::get().get_main_window(), WM_SIZE, SIZE_RESTORED, MAKELPARAM(rc.right - rc.left, rc.bottom - rc.top));
			}
			
			get().input_ = std::make_unique<input>();
			get().get_on_open().connect([]{
				gamelog::cout("Connected");
			});
			get().on_open_();
			// disable
			boost::signals2::signal<void()> empty {} ;
			std::swap(get().on_open_, empty);
			break;
	}
	case WM_DESTROY:
		{
			get().close();
			return 0;
		}
	case WM_NOTIFY:
		{
			
			// scintilla notifications
			if(const auto* n =reinterpret_cast<SCNotification *>(lp))
			{
				switch (n->updated)
				{
				case SC_UPDATE_H_SCROLL:
					{
						auto& e = get().get_engine();
						e->set_h_scroll(e->get_horizontal_scroll_offset() / e->get_char_width());
						break;
					}
				case SC_UPDATE_V_SCROLL:
					{
						auto& e = get().get_engine();
						e->set_v_scroll(e->get_first_visible_line());
						break;
					}
				default: break;
				}
				
			}
			
			break;
		}
	case WM_SIZING:
	case WM_SIZE:
		{
			const UINT width =  LOWORD(lp);
			const UINT height = HIWORD(lp);
			if(get().get_engine()) get().get_engine()->get_on_resize()(width, height );
			break;
		}
	default:
		break;
	}
	return CallWindowProc(reinterpret_cast<WNDPROC>(get().original_proc_), hwnd, msg, wp, lp);
}




bool notepader::hook_GetMessageW(const HMODULE module) const
{
	static decltype(&GetMessageW) original = nullptr;
	return iat_hook::hook_import<decltype(original)>(module,
	"user32.dll", "GetMessageW", original,
	[](const LPMSG lpMsg, const HWND hWnd, const UINT wMsgFilterMin, const UINT wMsgFilterMax) -> BOOL
	{

		if(const HWND stbar = FindWindowExW(notepader::get().get_main_window(), nullptr,  STATUSCLASSNAMEW, nullptr)){
			//gamelog::cout("valid");	
						}
		// TODO описание как из getmessage в peek message
		while(PeekMessageW(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, PM_REMOVE))
		{
			//gamelog::cout(std::hex, "get message", lpMsg->message);
			switch (lpMsg->message)
			{
			case WM_QUIT: return 0;
				
				// Handle keyboard messages
			case WM_KEYDOWN:
			case WM_KEYUP:
				{
					
					if(	lpMsg->wParam != VK_BACK
					&&  lpMsg->wParam != VK_DELETE)
					{
						if(auto& input = get().get_input_manager(); input) input->receive(lpMsg);
						// Block
						lpMsg->message = WM_NULL;
					}
					
					break;
				}
				
				// handle mouse
			case WM_MOUSEMOVE:
			case WM_LBUTTONDOWN:
			case WM_LBUTTONUP:
			case WM_RBUTTONDOWN:
			case WM_RBUTTONUP:
			case WM_LBUTTONDBLCLK:
			case WM_RBUTTONDBLCLK:
				{
					if(options::disable_mouse & get().options_) lpMsg->message = WM_NULL;
					break;
				}
			default: break;
			}
			
			TranslateMessage(lpMsg);                                     
			DispatchMessage(lpMsg);
			lpMsg->message = WM_NULL; // send the null to original dispatch loop
		}


		
		//try{
			static std::once_flag once;
			std::call_once(once, []{get().reset_to_start();});

			
			get().tickframe();
			
			//}
			//catch([[maybe_unused]] std::exception& e){
				//onError(e.what());
				//PostQuitMessage(0);
			//}
		
		return 1;
	});
}

bool notepader::hook_SendMessageW(const HMODULE module) const
{
	static decltype(&SendMessageW) original = nullptr;
	
	return iat_hook::hook_import<decltype(original)>(module,
	 "user32.dll", "SendMessageW", original,

	 [](HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) -> LRESULT
	 {
	     return original(hWnd, Msg, wParam, lParam);
	 });
}

bool notepader::hook_CreateWindowExW(HMODULE module) const
{
	static decltype(&CreateWindowExW) original = nullptr;
	return iat_hook::hook_import<decltype(original)>(module,
	"user32.dll", "CreateWindowExW", original,
	
	[](DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle,
	int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu,
	HINSTANCE hInstance, LPVOID lpParam) -> HWND {
		// here creates class 'Notepad' and 'Edit'; can change creation params here
	HWND out_hwnd;
		
	if (!lstrcmpW(lpClassName, WC_EDITW)) // handles the edit control creation and create custom window
	{
		
		get().engine_ = engine::create_new(get().options_);
		
		out_hwnd = get().engine_->create_native_window(dwExStyle,lpWindowName, dwStyle,
		X,Y,nWidth,nHeight,hWndParent,hMenu, hInstance,lpParam);
		
	}
	else
	{
		out_hwnd = original(dwExStyle, lpClassName, lpWindowName, dwStyle,
									 X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
	}
	
	// catch notepad.exe window
	if (!lstrcmpW(lpClassName, L"Notepad"))
	{
		
		get().main_window_ = out_hwnd;
		// patch the wnd proc
		get().original_proc_ = GetWindowLongPtr(get().main_window_, GWLP_WNDPROC);
		SetWindowLongPtr(get().main_window_, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(hook_wnd_proc));
		PostMessage(get().main_window_, WM_USER + 100, 0, 0);
	}
	
	return out_hwnd;
	});
}


bool notepader::hook_SetWindowTextW(HMODULE module) const
{
	static decltype(&SetWindowTextW) original = nullptr;
	return iat_hook::hook_import<decltype(original)>(module,
	"user32.dll", "SetWindowTextW", original,
	[](HWND hWnd, LPCWSTR lpString) -> BOOL {
	 return FALSE;
	});
}

void notepader::tickframe()
{
	engine_->get_world()->send();
	ticker::tickframe();
	set_window_title(L"notepadgame fps: " + std::to_wstring(get_current_frame_rate()));
	get().get_input_manager()->clear_input();
	
	
}
