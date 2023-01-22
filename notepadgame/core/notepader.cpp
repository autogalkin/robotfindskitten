#include <algorithm>
#include <locale>

#include <Windows.h>
#include "Richedit.h"
#include "CommCtrl.h"
#include "notepader.h"

#include <sstream>

#include "iat_hook.h"



void wtick();
DWORD main_post_init_notepad(); // from dllmain
DWORD mtick(LPVOID); // from dllmain
void notepader::init(const HWND& main_window)
{
	
	main_window_ = main_window;
	SetWindowText(main_window_, L"notepadgame");
	original_proc_ = GetWindowLongPtr(main_window_, GWLP_WNDPROC);
	// patch wnd proc
	SetWindowLongPtr(main_window_, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(hook_wnd_proc));
	get().input_ = std::make_unique<input>();
}

DWORD notepader::post_init_notepad_window(LPVOID) 
{
	//static COLORREF c = RGB(0, 255, 37);
	//static constexpr int i = COLOR_HIGHLIGHT;
	//SetSysColors(1, &i, &c); // try ti set highlight color, but it is sys color
	// set paragraph style
	//{
	//static PARAFORMAT2 pf;
	//pf.cbSize = sizeof(PARAFORMAT2);
	//pf.dwMask = PFM_LINESPACING ;
	//pf.bLineSpacingRule = 2;
	//SendMessage(get().get_world()->get_native_window(),EM_SETPARAFORMAT,0, reinterpret_cast<LPARAM>(&pf) );
	//}
	
	/*
	static RECT cr;
	cr.bottom = 0;
	cr.top = 0;
	cr.right = 100;
	cr.bottom = 100;
	SendMessage(get().get_world()->get_native_window(), EM_SETRECTNP,0, reinterpret_cast<LPARAM>(&cr) );
	*/
	//get().get_world()->set_selection();
	//get().get_world()->replace_selection(L"Hello");
	//get().get_world()->set_selection();

	
	/*
	static CHARFORMAT2A cf;
	cf.cbSize = sizeof(cf);
	cf.dwMask = CFM_KERNING | CFM_COLOR | CFM_EFFECTS ;
	cf.wKerning = 24;
	cf.crTextColor = RGB(255, 0, 0);
	cf.dwEffects = CFE_DISABLED;
	SendMessage(get().get_world()->get_native_window(), EM_SETCHARFORMAT,SCF_SELECTION SCF_WORD, reinterpret_cast<LPARAM>(& cf) );
	*/
	
	//SendMessage(get().get_world()->get_native_window(), EM_SETBKGNDCOLOR,0, RGB(37, 37, 38) );
	
	get().on_open_();
	// start game tick
	return main_post_init_notepad();
}

void notepader::close()
{
	on_close_();
	PostQuitMessage(0);
}

LRESULT notepader::hook_wnd_proc(HWND hwnd, const UINT msg, const WPARAM wp, const LPARAM lp)
{
	//outlog::get().print(std::hex, msg);
	switch (msg)
	{
	case WM_DESTROY:
		{
			get().close();
			break;
		}
	case WM_NOTIFY:
		{
			// обработка scintilla
			//auto* n =reinterpret_cast<SCNotification *>(lp);
			//if(n)
			//{
				//outputlog::mlog().print(n->position);
				
				//n->updated = SC_UPDATE_NONE;
				//return 0;
			//}
			//
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
		static auto initialized = 0l;
		if (InterlockedCompareExchange(&initialized, 1, 0) == 0)
		{
			if (const auto thread = CreateThread(nullptr, 0, post_init_notepad_window, nullptr, 0, nullptr)) {
				 CloseHandle(thread);
			 }
			//if (const auto thread = CreateThread(nullptr, 0, mtick, nullptr, 0, nullptr)) {
			//	 CloseHandle(thread);
			// }
		}
		//bool out = false;
		while(PeekMessageW(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, PM_REMOVE))
		{
			if (lpMsg->message == WM_QUIT)    
				return 0;
			
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
			TranslateMessage(lpMsg);                                     
			DispatchMessage(lpMsg);
			lpMsg->message = WM_NULL;
		}
		wtick();
		return 1;
		/*
		if(lpMsg->message == 99591)
		{
			outlog::get().print("in get message ", lpMsg->message);
		}
		
		// the first initialization call
		//std::call_once
		static auto initialized = 0l;
		if (InterlockedCompareExchange(&initialized, 1, 0) == 0)
		{
			if (const auto thread = CreateThread(nullptr, 0, post_init_notepad_window, nullptr, 0, nullptr)) {
		         CloseHandle(thread);
		     }
			if (const auto thread = CreateThread(nullptr, 0, mtick, nullptr, 0, nullptr)) {
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
		*/
	});
}

bool notepader::hook_SendMessageW(const HMODULE module) const
{
	static decltype(&SendMessageW) original = nullptr;
	
	return iat_hook::hook_import<decltype(original)>(module,
	 "user32.dll", "SendMessageW", original,

	 [](HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) -> LRESULT
	 {
	     //if(get().get_world())
	     //{
	         //if (const HWND& edit_window = get().get_world()->get_native_window();
	            
	             //hWnd == edit_window && Msg == EM_SETHANDLE) {
	             //auto mem = reinterpret_cast<HLOCAL>(wParam);
	             //auto buffer = reinterpret_cast<wchar_t*>(LocalLock(mem));
	             //LocalUnlock(mem);
	            // return NULL;
	         //}
	    // }
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

	// here creates class 'Notepad' and 'Edit'
		
	// can change creation params
	
	if (!lstrcmp(lpClassName, WC_EDIT)) // handles the edit control creation
	{
		
		HWND hwndEdit = get().init_world()->create_native_window(dwExStyle,lpWindowName, dwStyle,
		X,Y,nWidth,nHeight,hWndParent,hMenu, hInstance,lpParam);
		return hwndEdit;
	}
	
	
	const auto result = original(dwExStyle, lpClassName, lpWindowName, dwStyle,
		                             X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);

		
	// catch notepad.exe windows
	 if (!lstrcmpW(lpClassName, L"Notepad"))
	 {
	     get().init(result);
	 }

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
