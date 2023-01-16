#include <Windows.h>
#include <thread>

#include "character.h"
#include "input.h"
#include "core/common.h"




DWORD main_tick_func()
{
    
    constexpr double time_step = 1 / 10.0 * 100;
    notepad::get().get_input_manager()->init();
    character ch{L'f'};
    ch.bind_input();
    while (true)
    {
        on_global_tick();
        
        std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(time_step)));
    }
}



BOOL APIENTRY DllMain(const HMODULE h_module, const DWORD  ul_reason_for_call, LPVOID lp_reserved)
{
    // h_module is notepadgame.dll
    // np_module is notepad.exe
    
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    {
        // Ignore thread notifications
        DisableThreadLibraryCalls(h_module);
        
        // run the notepad singleton
        notepad& np = notepad::get();

        [[maybe_unused]] constexpr uint8_t start_options = notepad::hide_selection | notepad::kill_focus | notepad::disable_mouse;
        
        const auto np_module = GetModuleHandle(nullptr); // get the module handle of the notepad.exe
        np.connect_to_notepad(np_module); // start initialization and main tick
        
    }

    return TRUE;
}
