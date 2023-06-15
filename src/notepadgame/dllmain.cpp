#pragma warning(push, 0)
#include <Windows.h>
#include <stdint.h>
#pragma warning(pop)
#include "engine/notepader.h"
#include "engine/details/gamelog.h"
#include "engine/notepader.h"
#include "game/game.h"

BOOL APIENTRY DllMain(const HMODULE h_module, const DWORD  ul_reason_for_call, LPVOID lp_reserved)
{
    //  the h_module is notepadgame.dll and the np_module is notepad.exe
    
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    {   /**/
        // Ignore thread notifications
        DisableThreadLibraryCalls(h_module);
#ifndef NDEBUG
        gamelog::get(); // alloc a console for the cout
#endif // DEBUG    
        // run the notepad singleton
        notepader& np = notepader::get();
        const auto np_module = GetModuleHandleW(nullptr); // get the module handle of the notepad.exe
        
        [[maybe_unused]] constexpr uint8_t start_options =
            notepader::options::empty
#ifdef NDEBUG
            | notepader::hide_selection
            | notepader::kill_focus
            | notepader::disable_mouse          
#endif // NDEBUG  
            | notepader::show_eol    
            | notepader::show_spaces;
        
         np.get_on_open().connect([]{
            game::start();
        });
        np.connect_to_notepad(np_module, start_options); // start initialization and a game loop
       
    }
    return TRUE;
}
