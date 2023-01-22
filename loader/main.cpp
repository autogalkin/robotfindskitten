
#include <Windows.h>
#include <iostream>
#include <filesystem>
#include <string>

/*
* wchar_t buffer[MAX_PATH] = {};
    GetSystemDirectoryW(buffer,_countof(buffer));
    wcscat_s(buffer,L"\\notepad.exe");
    return std::wstring{buffer};
 */

int main(int argc, char* argv[])
{
    char cmd[256] = { 0 };
    snprintf(cmd, sizeof(cmd), "notepad");

    PROCESS_INFORMATION proc_info = { 0 };
    STARTUPINFOA startup_info = { 0 };
    startup_info.cb = sizeof(STARTUPINFO);
    if (!CreateProcessA(nullptr, cmd, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL,
        &startup_info, &proc_info)) {
        std::cerr << "Failed to launch Notepad" << std::endl;
        return 1;
    }

    const auto proc_handle = proc_info.hProcess;
    const auto proc_id = GetProcessId(proc_info.hProcess);
    std::cout << "Opened " << proc_id << std::endl;

    auto kernel32_handle = GetModuleHandleA("kernel32.dll");
    if (kernel32_handle == NULL) {
        std::cerr << "Failed to get kernel32" << std::endl;
        return 1;
    }

    auto LoadLibraryA_addr = GetProcAddress(kernel32_handle, "LoadLibraryA");
    if (LoadLibraryA_addr == NULL) {
        std::cerr << "Failed to get LoadLibraryA" << std::endl;
        return 1;
    }

    auto dll_path = std::filesystem::absolute("notepadgame.dll").string();
    
    auto buffer = VirtualAllocEx(proc_handle, NULL, MAX_PATH,
        MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (buffer == NULL) {
        std::cerr << "Failed to allocate buffer in remote process" << std::endl;
        return 1;
    }


    if (!WriteProcessMemory(proc_handle, buffer, dll_path.c_str(), MAX_PATH, NULL)) {
        std::cerr << "Failed to write to remote buffer" << std::endl;
        return 1;
    }
    
    
    auto remote_thread = CreateRemoteThread(proc_handle, NULL, 0,
        reinterpret_cast<LPTHREAD_START_ROUTINE>(LoadLibraryA_addr), buffer, NULL, NULL);
    if (remote_thread == NULL) {
        std::cerr << "Failed to create remote thread" << std::endl;
        return 1;
    }

    if (WaitForSingleObject(remote_thread, INFINITE) == WAIT_FAILED) {
        std::cerr << "Failed while waiting on remote thread to finish" << std::endl;
        return 1;
    }

    if (ResumeThread(proc_info.hThread) == -1) {
        std::cerr << "Failed to resume main process thread" << std::endl;
        return 1;
    }
    CloseHandle(proc_info.hThread);
    std::cout << "Done! thread_id=" << GetThreadId(remote_thread) << std::endl;
    printf("end");
    VirtualFreeEx(proc_handle, buffer, 0, MEM_RELEASE);
    
    CloseHandle(proc_handle);
   
    return 0;
}
