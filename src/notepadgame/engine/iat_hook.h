#pragma once
#pragma warning(push, 0)
#include "Windows.h"
#pragma warning(pop)



class iat_hook
{
    
public:
    
    static PIMAGE_IMPORT_DESCRIPTOR get_import_table(HMODULE module) {
        const auto dos_header = reinterpret_cast<PIMAGE_DOS_HEADER>(module);
        const auto nt_headers = reinterpret_cast<PIMAGE_NT_HEADERS>(
            reinterpret_cast<uintptr_t>(module) + dos_header->e_lfanew);
        if (nt_headers->Signature != IMAGE_NT_SIGNATURE) {
            return nullptr;
        }
        const auto [VirtualAddress, Size] = nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
        if (!VirtualAddress) {
            return nullptr;
        }
        return reinterpret_cast<PIMAGE_IMPORT_DESCRIPTOR>(
            reinterpret_cast<uintptr_t>(module) + VirtualAddress);
    }

    template<typename FuncType>
    static FuncType get_imported_function(char const* module_name, char const* func_name) {
        const auto module = GetModuleHandleA(module_name);
        if (!module) {
            return nullptr;
        }
        return reinterpret_cast<FuncType>(GetProcAddress(module, func_name));
    }
    
    template<typename FuncType>
    static bool hook_import(HMODULE module, char const* module_name, char const* func_name, FuncType& original, FuncType hook) {
        // Store a pointer to the actual function; this function can be called
        // multiple times in the same address space so we only want to do this
        // the first time
        if (!original) {
            original = get_imported_function<FuncType>(module_name, func_name);
            if (!original) {
                return false;
            }
        }

        auto import_descr = get_import_table(module);
        if (!import_descr) {
            return false;
        }

        // Walk the import descriptor table
        const auto base_addr = reinterpret_cast<uintptr_t>(module);
        for (; import_descr->Name != 0; import_descr++) {
            // Skip other modules

            if (const auto name = reinterpret_cast<LPCSTR>(base_addr + import_descr->Name)
                ; _stricmp(name, module_name) != 0) {
                continue;
            }

            auto thunk = reinterpret_cast<PIMAGE_THUNK_DATA>(base_addr + import_descr->FirstThunk);
            auto original_thunk = reinterpret_cast<PIMAGE_THUNK_DATA>(base_addr + import_descr->OriginalFirstThunk);
            for (; thunk->u1.Function != 0; thunk++, original_thunk++) {
                //auto import_by_name = reinterpret_cast<PIMAGE_IMPORT_BY_NAME>(base_addr + original_thunk->u1.AddressOfData);
                //auto curr_func_name = reinterpret_cast<char *>(import_by_name->Name);

                // Skip other functions
                if (thunk->u1.Function != reinterpret_cast<uintptr_t>(original)) {
                    continue;
                }

                // Patch the function pointer
                DWORD old_protection;
                VirtualProtect(thunk, sizeof(PIMAGE_THUNK_DATA), PAGE_READWRITE, &old_protection);
                thunk->u1.Function = reinterpret_cast<uintptr_t>(hook);
                VirtualProtect(thunk, sizeof(PIMAGE_THUNK_DATA), old_protection, &old_protection);
            }
        }
        return true;
    }

};
