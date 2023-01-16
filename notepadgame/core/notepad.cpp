#include <algorithm>
#include <locale>

#include <Windows.h>
#include "CommCtrl.h"
#include "notepad.h"

#include <sstream>

#include "iat_hook.h"




DWORD main_tick_func(); // from dllmain 

void notepad::init(const HWND& main_window)
{
	
	main_window_ = main_window;
	SetWindowText(main_window_, L"notepadgame");
	original_proc_ = GetWindowLongPtr(main_window_, GWLP_WNDPROC);
	// patch wnd proc
	SetWindowLongPtr(main_window_, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(hook_wnd_proc));
	get().input_ = std::make_unique<input>();
}

DWORD notepad::post_init_notepad_window(LPVOID) 
{
	//static COLORREF c = RGB(0, 255, 37);
	//static constexpr int i = COLOR_HIGHLIGHT;
	//SetSysColors(1, &i, &c); // try ti set highlight color, but it is sys color
	
	
	get().on_open_();
	// start game tick
	return main_tick_func();
}

void notepad::close()
{
	on_close_();
	PostQuitMessage(0);
}

LRESULT notepad::hook_wnd_proc(HWND hwnd, const UINT msg, const WPARAM wp, const LPARAM lp)
{

	switch (msg)
	{
	case WM_DESTROY:
		{
			get().close();
			return 0;
		}
	case WM_CTLCOLOREDIT:
		// change the edit control color
		return world::set_window_color(reinterpret_cast<HDC>(wp));
	

	default:
		break;
	}
	return CallWindowProc(reinterpret_cast<WNDPROC>(get().original_proc_), hwnd, msg, wp, lp);
}




bool notepad::hook_GetMessageW(HMODULE module) const
{
	static decltype(&GetMessageW) original = nullptr;
	return iat_hook::hook_import<decltype(original)>(module,
	"user32.dll", "GetMessageW", original,
	[](LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax) -> BOOL
	{
		
		// the first initialization call
		static auto initialized = 0l;
		if (InterlockedCompareExchange(&initialized, 1, 0) == 0)
		{
			if (const auto thread = CreateThread(nullptr, 0, post_init_notepad_window, nullptr, 0, nullptr)) {
		         CloseHandle(thread);
		     }
		}
			
		if(get().get_world())
		{
		    // Constantly try to kill the edit control's focus
			//HideCaret(get().get_world()->get_native_window());
		   if(options::kill_focus & get().start_options_)
		   {
		   	  SendMessage(get().get_world()->get_native_window(), WM_KILLFOCUS, 0, 0);
		   }
		   	
			
		}

		const auto orig_res = original(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);
		
		 // Handle keyboard messages
		 if (lpMsg->message == WM_KEYDOWN || lpMsg->message == WM_KEYUP)
		 {

		 	if(		 lpMsg->wParam != VK_UP
				 &&  lpMsg->wParam != VK_DOWN
				 &&  lpMsg->wParam != VK_BACK
				 &&  lpMsg->wParam != VK_DELETE)
		 	{
		 		if(auto& input = get().get_input_manager()) input->set_input_message(lpMsg);
		 		// Block messages
				 lpMsg->message = WM_NULL;
		 	}
		 }
		if(options::disable_mouse & get().start_options_)
		{
			if(     lpMsg->message == WM_MOUSEMOVE			
				 || lpMsg->message == WM_LBUTTONDOWN		
				 || lpMsg->message == WM_LBUTTONUP			
				 || lpMsg->message == WM_RBUTTONDOWN		
				 || lpMsg->message == WM_RBUTTONUP			
				 || lpMsg->message == WM_LBUTTONDBLCLK		
				 || lpMsg->message == WM_RBUTTONDBLCLK)
			{
				lpMsg->message = WM_NULL;
			}
		}
		return orig_res;
	});
}

bool notepad::hook_SendMessageW(const HMODULE module) const
{
	static decltype(&SendMessageW) original = nullptr;
	
	return iat_hook::hook_import<decltype(original)>(module,
	 "user32.dll", "SendMessageW", original,

	 [](HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) -> LRESULT
	 {
	     if(get().get_world())
	     {
	         if (const HWND& edit_window = get().get_world()->get_native_window();
	            
	             hWnd == edit_window && Msg == EM_SETHANDLE) {
	             //auto mem = reinterpret_cast<HLOCAL>(wParam);
	             //auto buffer = reinterpret_cast<wchar_t*>(LocalLock(mem));
	             //LocalUnlock(mem);
	             return NULL;
	         }
	     }
	     return original(hWnd, Msg, wParam, lParam);
	 });
}

bool notepad::hook_CreateWindowExW(HMODULE module) const
{
	static decltype(&CreateWindowExW) original = nullptr;
	return iat_hook::hook_import<decltype(original)>(module,
	"user32.dll", "CreateWindowExW", original,
	
	[](DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle,
	int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu,
	HINSTANCE hInstance, LPVOID lpParam) -> HWND {

	// can change creation params
	if (!lstrcmp(lpClassName, WC_EDIT))
	{
		if(options::hide_selection & get().start_options_) dwStyle +=  ES_NOHIDESEL ; // hides selection color visible, work with WM_KILLFOCUS
	}
	
	 auto result = original(dwExStyle, lpClassName, lpWindowName, dwStyle,
	     X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);

	// catch notepad.exe windows
	 if (!lstrcmpW(lpClassName, L"Notepad"))
	 {
	     get().init(result);
	 }
	 else if (!lstrcmp(lpClassName, WC_EDIT))
	 {
	 		
	     get().init_world(result);
	 }

	 return result;
	});
}

bool notepad::hook_SetWindowTextW(HMODULE module) const
{
	static decltype(&SetWindowTextW) original = nullptr;
	return iat_hook::hook_import<decltype(original)>(module,
	"user32.dll", "SetWindowTextW", original,
	[](HWND hWnd, LPCWSTR lpString) -> BOOL {
	 return FALSE;
	});
}
