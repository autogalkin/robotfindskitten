#include <algorithm>
#include <locale>

#include <Windows.h>
#include "Richedit.h"
#include "CommCtrl.h"
#include "notepader.h"

#include <mutex>


#include "iat_hook.h"




void notepader::init(const HWND& main_window)
{
	
	main_window_ = main_window;
	SetWindowText(main_window_, L"notepadgame");
	
	// patch wnd proc
	original_proc_ = GetWindowLongPtr(main_window_, GWLP_WNDPROC);
	SetWindowLongPtr(main_window_, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(hook_wnd_proc));
	
}

void notepader::post_connect_to_notepad() 
{
	
	get().input_ = std::make_unique<input>();
	get().get_input_manager()->init();
	// TODO убрать по местам
	SendMessage(notepader::get().get_world()->get_native_window(), SCI_SETVIRTUALSPACEOPTIONS, SCVS_USERACCESSIBLE|SCVS_NOWRAPLINESTART, 0);
	SendMessage(notepader::get().get_world()->get_native_window(), SCI_SETADDITIONALSELECTIONTYPING, 1, 0);
        
	auto& world = notepader::get().get_world();
	notepader::get().get_world()->show_spaces(true);
	notepader::get().get_world()->show_eol(true);
    
	world->set_background_color(RGB(37,37,38));
	world->set_all_text_color(RGB(240,240,240));
    
	//SendMessage(world->get_native_window(), SCI_HIDESELECTION, 1,0);
	// set font
	SendMessage(world->get_native_window(), SCI_STYLESETFONT, STYLE_DEFAULT, reinterpret_cast<sptr_t>("Lucida Console"));
	SendMessage(world->get_native_window() ,SCI_STYLESETBOLD, STYLE_DEFAULT, 1); // bold
	SendMessage(world->get_native_window(), SCI_STYLESETSIZE, STYLE_DEFAULT,36); // pt size
	SendMessage(world->get_native_window(), SCI_STYLESETCHECKMONOSPACED, STYLE_DEFAULT,1);
	SendMessage(world->get_native_window(), SCI_SETHSCROLLBAR, 1,0);
	
	get().on_open_();
	
}

void notepader::close()
{
	on_close_();
	PostQuitMessage(0);
}

LRESULT notepader::hook_wnd_proc(HWND hwnd, const UINT msg, const WPARAM wp, const LPARAM lp)
{
	switch (msg)
	{
	case WM_DESTROY:
		{
			get().close();
			return 0;
		}
	case WM_NOTIFY:
		{
			// обработка scintilla auto* n =reinterpret_cast<SCNotification *>(lp);
			break;
		}
	default:
		break;
	}
	return CallWindowProc(reinterpret_cast<WNDPROC>(get().original_proc_), hwnd, msg, wp, lp);
}




bool notepader::hook_GetMessageW(HMODULE module) const
{
	static decltype(&GetMessageW) original = nullptr;
	return iat_hook::hook_import<decltype(original)>(module,
	"user32.dll", "GetMessageW", original,
	[](LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax) -> BOOL
	{
		
		while(PeekMessageW(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, PM_REMOVE))
		{
			if (lpMsg->message == WM_QUIT)    
				return 0;
			
			// Handle keyboard messages
			if (lpMsg->message == WM_KEYDOWN || lpMsg->message == WM_KEYUP)
			{

				if(		lpMsg->wParam != VK_UP
					&&  lpMsg->wParam != VK_DOWN
					&&  lpMsg->wParam != VK_BACK
					&&  lpMsg->wParam != VK_DELETE)
				{
					if(auto& input = get().get_input_manager()) input->set_input_message(lpMsg);
					// Block messages
					lpMsg->message = WM_NULL;
				}
			}
			// handle mouse
			if(options::disable_mouse & get().options_)
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
			TranslateMessage(lpMsg);                                     
			DispatchMessage(lpMsg);
			lpMsg->message = WM_NULL; // send null to original dispatch loop
		}

		
		get().tickframe(1); // TODO delta time
		
		return 1;
		//const auto orig_res = original(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);
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
	if (!lstrcmp(lpClassName, WC_EDIT)) // handles the edit control creation and create custom window
	{
		HWND hwndEdit = get().init_world()->create_native_window(dwExStyle,lpWindowName, dwStyle,
		X,Y,nWidth,nHeight,hWndParent,hMenu, hInstance,lpParam);
		return hwndEdit;
	}
		
	const auto result = original(dwExStyle, lpClassName, lpWindowName, dwStyle,
		                             X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
		
	// catch notepad.exe window
	if (!lstrcmp(lpClassName, L"Notepad"))
	{
	     get().init(result);
	}
		
	if(get().get_main_window() && get().get_world()->get_native_window()) post_connect_to_notepad();
	
	return result;
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
