
#include <Windows.h>
#include <stdint.h>

#include "engine/notepad.h"
#include "game/game.h"

extern constexpr int GAME_AREA[2] = {150, 300};

class console final : public noncopyable, public nonmoveable {
  public:
    static console allocate(); 
    ~console(){
        FreeConsole();
    }
  private:
    explicit console(){};

};

inline console console::allocate() {
    AllocConsole();
    FILE* fdummy;
    auto err = freopen_s(&fdummy, "CONOUT$", "w", stdout);
    err = freopen_s(&fdummy, "CONOUT$", "w", stderr);
    std::cout.clear();
    std::clog.clear();
    const HANDLE hConOut =
        CreateFileW(L"CONOUT$", GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL, NULL);
    SetStdHandle(STD_OUTPUT_HANDLE, hConOut);
    SetStdHandle(STD_ERROR_HANDLE, hConOut);
    std::wcout.clear();
    std::wclog.clear();
    return console();
}

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
            static auto log_console = console::allocate();
#endif // NDEBUG
            printf("Notepad is loaded and initialized. Start a game");
            game::start(world, i, cmds, GAME_AREA);
        });

        constexpr int world_widht = 300;
        constexpr int world_height = 100;

        np.connect_to_notepad(
            np_module, start_options); // start initialization and a game loop
    }
    return TRUE;
}
