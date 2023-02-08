


#include <Windows.h>





#include "core/gamelog.h"
#include "core/notepader.h"
#include "core/engine.h"


#include "range/v3/view/enumerate.hpp"

#include "range/v3/view/join.hpp"


#include <range/v3/view/filter.hpp>

#include "entities/atmosphere.h"
#include "entities/character.h"

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
            const auto ch = w->get_world()->spawn_actor<character>(location{1,2}, 'f');
            const auto ch2 = w->get_world()->spawn_actor<character>(location{7,15}, 's');
            using namespace std::chrono_literals;
             const auto atm = w->get_world()->spawn_actor<atmosphere>(30s
                 ,  color_range{RGB(37,37,38),RGB(240,240,240) }
                 ,  color_range{RGB(220,220,220),RGB(37,37,38) });
            
            ch->bind_input();
            notepader::get().get_input_manager()->get_down_signal().connect([](const input::key_state_type& state)
            {
                auto& w = notepader::get().get_world();
                std::string s{"👀"};
                //notepader::get().get_world()->insert_text(2, s);
               
                //
            });
            //const auto ch3 = w->get_level()->spawn_actor<animated_mesh>({7,15}, 'f', 'd', 'h');
            
        });
    }
    return TRUE;
}
