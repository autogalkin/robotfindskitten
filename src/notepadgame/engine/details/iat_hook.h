#pragma once

#include "Windows.h"
#include <string_view>


class iat_hook {

  public:
    static PIMAGE_IMPORT_DESCRIPTOR get_import_table(HMODULE module) {
        auto *const dos_header = reinterpret_cast<PIMAGE_DOS_HEADER>(module);
        auto *const nt_headers = reinterpret_cast<PIMAGE_NT_HEADERS>(
            reinterpret_cast<uintptr_t>(module) + dos_header->e_lfanew);
        if (nt_headers->Signature != IMAGE_NT_SIGNATURE) {
            return nullptr;
        }
        const auto [VirtualAddress, Size] =
            nt_headers->OptionalHeader
                .DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
        if (!VirtualAddress) {
            return nullptr;
        }
        return reinterpret_cast<PIMAGE_IMPORT_DESCRIPTOR>(
            reinterpret_cast<uintptr_t>(module) + VirtualAddress);
    }

    struct modulename{
        std::string_view name;
    };
    struct funcname{
        std::string_view name;
    };
    template <typename FuncType>
    static FuncType get_imported_function(const modulename module_name,
                                          const funcname func_name) {
        auto *const module_handle = GetModuleHandleA(module_name.name.data());
        if (!module_handle) {
            return nullptr;
        }
        return reinterpret_cast<FuncType>(GetProcAddress(module_handle, func_name.name.data()));
    }

    template <typename FuncType>
    static bool hook_import(HMODULE module, modulename module_name,
                            funcname func_name, FuncType& original,
                            FuncType hook) {
        // Store a pointer to the actual function; this function can be called
        // multiple times in the same address space so we only want to do this
        // the first time
        if (!original) {
            original = get_imported_function<FuncType>(module_name, func_name);
            if (!original) {
                return false;
            }
        }

        auto *import_descr = get_import_table(module);
        if (!import_descr) {
            return false;
        }
        ;
        // Walk the import descriptor table
        const auto base_addr = reinterpret_cast<uintptr_t>(module);
        for (; import_descr->Name != 0; std::advance(import_descr, 1)) {
            // Skip other modules
            if (const auto *const name =
                    reinterpret_cast<LPCSTR>(base_addr + import_descr->Name);
                name == module_name.name) {
                continue;
            }

            auto *thunk = reinterpret_cast<PIMAGE_THUNK_DATA>(
                base_addr + import_descr->FirstThunk);
            auto *original_thunk = reinterpret_cast<PIMAGE_THUNK_DATA>(
                base_addr + import_descr->OriginalFirstThunk);
            for (; thunk->u1.Function != 0; thunk++, original_thunk++) {
                // Skip other functions
                if (thunk->u1.Function !=
                    reinterpret_cast<uintptr_t>(original)) {
                    continue;
                }

                // Patch the function pointer
                DWORD old_protection;
                VirtualProtect(thunk, sizeof(PIMAGE_THUNK_DATA), PAGE_READWRITE,
                               &old_protection);
                thunk->u1.Function = reinterpret_cast<uintptr_t>(hook);
                VirtualProtect(thunk, sizeof(PIMAGE_THUNK_DATA), old_protection,
                               &old_protection);
            }
        }
        return true;
    }
};
