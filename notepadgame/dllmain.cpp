


#include <Windows.h>

#include "core/gamelog.h"
#include "core/notepader.h"
#include "core/world.h"

#include "world/character.h"
#include "world/level.h"


BOOL APIENTRY DllMain(const HMODULE h_module, const DWORD  ul_reason_for_call, LPVOID lp_reserved)
{
    //  the h_module is notepadgame.dll and the np_module is notepad.exe
    
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    {
        // Ignore thread notifications
        DisableThreadLibraryCalls(h_module);

        gamelog::get(); // alloc a console for the cout
        
        // run the notepad singleton
        notepader& np = notepader::get();
        const auto np_module = GetModuleHandle(nullptr); // get the module handle of the notepad.exe
        
        [[maybe_unused]] constexpr uint8_t start_options =
            0
            //  notepader::hide_selection
            //| notepader::kill_focus
            //| notepader::disable_mouse
            | notepader::show_eol
            | notepader::show_spaces;
        
        np.connect_to_notepad(np_module, start_options); // start initialization and a game loop
        
        np.get_on_open().connect([]
        {
            auto& w = notepader::get().get_world();
            w->set_background_color(RGB(37,37,38));
            w->set_all_text_color(RGB(240,240,240));
            const auto ch = w->get_level()->spawn_actor<character>(translation{1,2}, 'f');
            const auto ch2 = w->get_level()->spawn_actor<character>(translation{7,15}, 's');
            ch->bind_input();
        });
    }
    return TRUE;
}
