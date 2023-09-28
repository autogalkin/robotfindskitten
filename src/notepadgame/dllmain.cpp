
#include <Windows.h>
#include <stdint.h>

#include "engine/notepad.h"
#include "game/game.h"

extern constexpr int GAME_AREA[2] = {100, 150};

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
#ifndef NDEBUG
            | notepad::opts::show_eol 
#endif // NDEBUG
            | notepad::opts::hide_selection 
            | notepad::opts::show_spaces;

        np.on_open()->get().connect([](world& world, back_buffer& b, notepad::commands_queue_t& cmds) {
#ifndef NDEBUG
            static auto log_console = console::allocate();
#endif // NDEBUG
            printf("Notepad is loaded and initialized. Start a game\n");
            game::start(world, b, cmds, GAME_AREA);
        });

        np.connect_to_notepad(
            np_module, start_options); // start initialization and a game loop
    }
    return TRUE;
}
