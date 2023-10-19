#include <Windows.h>
#include <consoleapi.h>
#include <cstdint>

#include "engine/notepad.h"
#include "engine/details/base_types.hpp"
#include "game/game.h"

static constexpr pos GAME_AREA = {150, 100};

namespace console {
inline std::unique_ptr<void, void (*)(void*)> allocate() {
    AllocConsole();
    FILE* fdummy = nullptr;
    [[maybe_unused]] auto err = freopen_s(&fdummy, "CONOUT$", "w", stdout);
    err = freopen_s(&fdummy, "CONOUT$", "w", stderr);
    std::cout.clear();
    std::clog.clear();
    HANDLE hConOut = CreateFileW(L"CONOUT$", GENERIC_READ | GENERIC_WRITE,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                                 OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    SetStdHandle(STD_OUTPUT_HANDLE, hConOut);
    SetStdHandle(STD_ERROR_HANDLE, hConOut);
    std::wcout.clear();
    std::wclog.clear();
    return {nullptr, [](void*) { FreeConsole(); }};
}
} // namespace console

BOOL APIENTRY DllMain(const HMODULE h_module, const DWORD ul_reason_for_call,
                      LPVOID /*lp_reserved*/) {
    //  the h_module is the robotfindskitten.dll

    if(ul_reason_for_call == DLL_PROCESS_ATTACH) {
        // Ignore thread notifications
        DisableThreadLibraryCalls(h_module);

        // run the notepad singleton
        notepad& np = notepad::get();
        auto* const np_module = GetModuleHandleW(
            nullptr); // get the module handle of the notepad.exe

        [[maybe_unused]] constexpr notepad::opts start_options =
            notepad::opts::empty
#ifndef NDEBUG
            | notepad::opts::show_eol
#endif
            | notepad::opts::show_spaces;

        // Optional always valid here
        // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
        np.on_open()->get().connect([](auto shutdown) {
#ifndef NDEBUG
            static auto log_console = console::allocate();
#endif
            std::cout << ("Notepad is loaded and initialized. Start a game\n");
            game::start(GAME_AREA, std::move(shutdown));
        });

        np.connect_to_notepad(
            np_module, start_options); // start initialization and the game loop
    }
    return TRUE;
}
