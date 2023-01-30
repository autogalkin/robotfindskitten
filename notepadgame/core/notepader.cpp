#include <algorithm>
#include <locale>

#include <Windows.h>
#include "Richedit.h"
#include "CommCtrl.h"
#include "notepader.h"

#include <mutex>

#include "../world_entities/character.h"
#include "iat_hook.h"




void notepader::init(const HWND& main_window)
{
	
	main_window_ = main_window;
	
	// patch wnd proc
	original_proc_ = GetWindowLongPtr(main_window_, GWLP_WNDPROC);
	SetWindowLongPtr(main_window_, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(hook_wnd_proc));
	
}

void notepader::post_connect_to_notepad() 
{
	get().input_ = std::make_unique<input>();
	
	//SendMessage(notepader::get().get_world()->get_native_window(), SCI_SETVIRTUALSPACEOPTIONS, SCVS_USERACCESSIBLE|SCVS_NOWRAPLINESTART, 0);
	//SendMessage(notepader::get().get_world()->get_native_window(), SCI_SETADDITIONALSELECTIONTYPING, 1, 0);
        
	auto& world = notepader::get().get_world();
	world->show_spaces(true);
	world->show_eol(true);
	world->set_background_color(RGB(37,37,38));
	world->set_all_text_color(RGB(240,240,240));
	get().on_open_();

	
	//get().get_input_manager()->get_down_signal().connect([](const input::key_state_type state)
	//{
	//	if(state.size() && state[0] == input::key::d)
	//	{
			
			//char* ptr = reinterpret_cast<char*>(SendMessage(get().get_world()->get_native_window(), SCI_GETRANGEPOINTER, 1, 5));
			//ptr[1] = 'g';
			//auto ch = "Hello World";
			//SendMessage(get().get_world()->get_native_window(), SCI_GETRANGEPOINTER, 1, reinterpret_cast<LPARAM>(ch));
			
	//	}
	//});
	
	const auto ch = get().get_world()->level->spawn_actor<character>(translation{1,2}, 'f');
	const auto ch2 = get().get_world()->level->spawn_actor<character>(translation{7,15}, 's');
	ch->bind_input();
	
	
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
			
			// scintilla notifications
			if(const auto* n =reinterpret_cast<SCNotification *>(lp))
			{
				switch (n->updated)
				{
				case SC_UPDATE_H_SCROLL:
					{
						auto& w = get().get_world();
						const auto& scroll = w->level->scroll.get();
						w->level->scroll.pin().index_in_line(w->get_horizontal_scroll_offset() / w->get_char_width());
						
						break;
					}
				case SC_UPDATE_V_SCROLL:
					{
						get().get_world()->level->scroll.pin().line(get().get_world()->get_first_visible_line());
						break;
					}
				default: break;
				}
				
			}
			
			break;
		}
	default:
		break;
	}
	return CallWindowProc(reinterpret_cast<WNDPROC>(get().original_proc_), hwnd, msg, wp, lp);
}



// TODO описание как из getmessage в peek message
bool notepader::hook_GetMessageW(const HMODULE module) const
{
	static decltype(&GetMessageW) original = nullptr;
	return iat_hook::hook_import<decltype(original)>(module,
	"user32.dll", "GetMessageW", original,
	[](LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax) -> BOOL
	{
		
		while(PeekMessageW(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, PM_REMOVE))
		{
			//TODO switch
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
					if(auto& input = get().get_input_manager()) input->receive(lpMsg);
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

		try{
			static  std::once_flag once;
			std::call_once(once, []{get().reset_to_start();});
			get().tickframe();
			
			}
			catch([[maybe_unused]] std::exception& e){
				//onError(e.what());
				PostQuitMessage(0);
				//close(); // TODO handle exeptions
			}
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
	HWND out_hwnd;
		
	if (!lstrcmp(lpClassName, WC_EDIT)) // handles the edit control creation and create custom window
	{
		out_hwnd = get().make_world()->create_native_window(dwExStyle,lpWindowName, dwStyle,
		X,Y,nWidth,nHeight,hWndParent,hMenu, hInstance,lpParam);
	}
	else
	{
		out_hwnd = original(dwExStyle, lpClassName, lpWindowName, dwStyle,
									 X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
	}
		
	// catch notepad.exe window
	if (!lstrcmp(lpClassName, L"Notepad"))
	{
	     get().init(out_hwnd);
	}
	
	if(get().get_main_window() && get().get_world() && get().get_world()->get_native_window())
	{
		static std::once_flag once;
		std::call_once(once, post_connect_to_notepad);
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
	world_->level->send();
	ticker::tickframe();
	set_window_title(L"notepadgame fps: " + std::to_wstring(get_current_frame_rate()));
	//world_->backbuffer->get();
	
}
