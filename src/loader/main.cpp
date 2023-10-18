#include <array>
#include <filesystem>
#include <format>
#include <functional>
#include <iostream>
#include <string>

#include <Windows.h>

#include "config.h"

class dll_injection_error final: public std::runtime_error {
public:
    explicit dll_injection_error(const std::string& message)
        : runtime_error(message) {}
};
[[noreturn]] void error(const char* message) {
    throw dll_injection_error(
        std::format("[!] Err `{}` = {}\n", GetLastError(), message));
}

int main(int /*argc*/, char* /*argv*/[]) {
    try {
        std::unique_ptr<PROCESS_INFORMATION, void (*)(PROCESS_INFORMATION*)>
            proc_info{new PROCESS_INFORMATION{}, [](PROCESS_INFORMATION* ptr) {
                          CloseHandle(ptr->hThread);
                          CloseHandle(ptr->hProcess);
                      }};

        std::cout << "Launch notepad.exe..." << std::endl;
        {
            std::string cmd = std::format(
                "{}\\notepad.exe",
                std::getenv("windir")); // NOLINT(concurrency-mt-unsafe)
            STARTUPINFOA startup_info{};
            ZeroMemory(&startup_info, sizeof(startup_info));
            startup_info.cb = sizeof(STARTUPINFOA);
            ZeroMemory(proc_info.get(), sizeof(PROCESS_INFORMATION));
            // Causes Asan error `heap-use-after-free`
            // in the function ntdll.dll`RtlCreateProcessParametersWithTemplate
            // if `lpCommandLine` parameter is nullptr
            auto dummy = std::string(2, ' ');
            if(!CreateProcessA(static_cast<LPCSTR>(cmd.c_str()),
                               dummy.data(), // lpCommandLine
                               nullptr, nullptr, FALSE, CREATE_SUSPENDED,
                               nullptr, nullptr, &startup_info,
                               proc_info.get())) {
                error("Launch notepad.exe process..");
            }
        }

        std::cout << "`Notepad process id is "
                  << GetProcessId(proc_info->hProcess) << std::endl;

        auto* const kernel32_handle = GetModuleHandleA("kernel32.dll");
        if(!kernel32_handle) {
            error("Get kernel32");
        }

        const auto LoadLibraryA_addr =
            GetProcAddress(kernel32_handle, "LoadLibraryA");
        if(!LoadLibraryA_addr) {
            error("Failed to get LoadLibraryA");
        }

        const auto dll_path =
            std::filesystem::absolute(PROJECT_NAME ".dll").string();

        const std::unique_ptr<std::remove_pointer_t<LPVOID>,
                              std::function<void(LPVOID)>>
            dll_addr{

                VirtualAllocEx(proc_info->hProcess, nullptr, MAX_PATH,
                               MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE),
                [&proc_info](LPVOID addr) {
                    VirtualFreeEx(proc_info->hProcess, addr, 0, MEM_RELEASE);
                }};

        if(!dll_addr) {
            error("Failed to allocate buffer in remote process");
        }

        if(!WriteProcessMemory(proc_info->hProcess, dll_addr.get(),
                               dll_path.c_str(), MAX_PATH, nullptr)) {
            error("Failed to write to remote buffer");
        }

        auto* const remote_thread = CreateRemoteThread(
            proc_info->hProcess, nullptr, 0,
            reinterpret_cast<LPTHREAD_START_ROUTINE>(
                LoadLibraryA_addr) // NOLINT(clang-diagnostic-cast-function-type)
            ,
            dll_addr.get(), NULL, nullptr);
        if(!remote_thread) {
            error("Failed to create remote thread");
        }

        if(WaitForSingleObject(remote_thread, INFINITE) == WAIT_FAILED) {
            error("Failed while waiting on remote thread to finish");
        }

        if(ResumeThread(proc_info->hThread) == static_cast<DWORD>(-1)) {
            error("Failed to resume main process thread");
        }

        std::cout << "Done! the robotfindskitten dll thread_id="
                  << GetThreadId(remote_thread) << std::endl;

    } catch(const dll_injection_error& err) {
        std::cerr << err.what();
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
