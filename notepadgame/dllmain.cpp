


#include <Windows.h>
#include "core/notepader.h"


BOOL APIENTRY DllMain(const HMODULE h_module, const DWORD  ul_reason_for_call, LPVOID lp_reserved)
{
    //  the h_module is notepadgame.dll and the np_module is notepad.exe
    
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    {
        // Ignore thread notifications
        DisableThreadLibraryCalls(h_module);
        
        // run the notepad singleton
        notepader& np = notepader::get();
        const auto np_module = GetModuleHandle(nullptr); // get the module handle of the notepad.exe
        
        [[maybe_unused]] constexpr uint8_t start_options = notepader::hide_selection | notepader::kill_focus | notepader::disable_mouse;
        
        np.connect_to_notepad(np_module); // start initialization and a game loop
    }
    return TRUE;
}
