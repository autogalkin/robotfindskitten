#pragma warning(push, 0)
#include <Windows.h>
#include <stdint.h>
#pragma warning(pop)
#include "engine/details/gamelog.h"
#include "engine/notepad.h"
#include "game/game.h"

BOOL APIENTRY DllMain(const HMODULE h_module, const DWORD ul_reason_for_call,
                      LPVOID lp_reserved) {
    //  the h_module is notepadgame.dll


    if (ul_reason_for_call == DLL_PROCESS_ATTACH) { 
        // Ignore thread notifications
        DisableThreadLibraryCalls(h_module);
#ifndef NDEBUG
        gamelog::get(); // alloc a console for the cout
#endif // NDEBUG
       // run the notepad singleton
        notepad_t& np = notepad_t::get();
        const auto np_module = GetModuleHandleW(
            nullptr); // get the module handle of the notepad.exe

        [[maybe_unused]] constexpr notepad_t::opts start_options =
            notepad_t::opts::empty
#ifdef NDEBUG
            | notepad_t::opts::hide_selection | notepad_t::opts::kill_focus |
            notepad_t::opts::disable_mouse
#endif // NDEBUG
            | notepad_t::opts::show_eol | notepad_t::opts::show_spaces;

        np.on_open()->get().connect([] { game::start(); });
        np.connect_to_notepad(
            np_module, start_options); // start initialization and a game loop
    }
    return TRUE;
}
