
#include <Windows.h>
#include <stdint.h>

#include "engine/details/gamelog.h"
#include "engine/notepad.h"
#include "game/game.h"

BOOL APIENTRY DllMain(const HMODULE h_module, const DWORD ul_reason_for_call,
                      LPVOID lp_reserved) {
    //  the h_module is notepadgame.dll

    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        // Ignore thread notifications
        DisableThreadLibraryCalls(h_module);

        // run the notepad singleton
        notepad& np = notepad::get();
        const auto np_module = GetModuleHandleW(
            nullptr); // get the module handle of the notepad.exe

        [[maybe_unused]] constexpr notepad::opts start_options =
            notepad::opts::empty
#ifdef NDEBUG
            | notepad_t::opts::hide_selection | notepad_t::opts::kill_focus |
            notepad_t::opts::disable_mouse
#endif // NDEBUG
            | notepad::opts::show_eol | notepad::opts::show_spaces;

        np.on_open()->get().connect([](world& world, input::thread_input& i,  thread_commands& cmds) {
#ifndef NDEBUG
            gamelog::get(); // alloc a console for the cout
#endif // NDEBUG
            printf("Notepad is loaded and initialized. Start a game");
            game::start(world, i, cmds);
        });

        constexpr int world_widht = 300;
        constexpr int world_height = 100;

        np.connect_to_notepad(
            np_module, start_options); // start initialization and a game loop
    }
    return TRUE;
}
