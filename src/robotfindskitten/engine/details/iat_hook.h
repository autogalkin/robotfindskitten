
#pragma once
#ifndef _CPP_PROJECTS_ROBOTFINDSKITTEN_SRC_ROBOTFINDSKITTEN_ENGINE_DETAILS_IAT_HOOK_H
#define _CPP_PROJECTS_ROBOTFINDSKITTEN_SRC_ROBOTFINDSKITTEN_ENGINE_DETAILS_IAT_HOOK_H

#include <string_view>

#include "Windows.h"

namespace iat_hook {

[[nodiscard]] inline PIMAGE_IMPORT_DESCRIPTOR get_import_table(HMODULE module) {
    auto* const dos_header = reinterpret_cast<PIMAGE_DOS_HEADER>(module);
    auto* const nt_headers = reinterpret_cast<PIMAGE_NT_HEADERS>(
        reinterpret_cast<uintptr_t>(module) + dos_header->e_lfanew);
    if(nt_headers->Signature != IMAGE_NT_SIGNATURE) {
        return nullptr;
    }
    const auto [virtual_addr, Size] =
        nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if(!virtual_addr) {
        return nullptr;
    }
    return reinterpret_cast<PIMAGE_IMPORT_DESCRIPTOR>(
        reinterpret_cast<uintptr_t>(module) + virtual_addr);
}

struct module_name: std::string_view {
    explicit module_name(const std::string_view& s): std::string_view(s) {}
    explicit module_name(std::string_view&& s): std::string_view(s) {}
};
struct function_name: std::string_view {
    explicit function_name(const std::string_view& s): std::string_view(s) {}
    explicit function_name(std::string_view&& s): std::string_view(s) {}
};
template<typename FuncType>
[[nodiscard]] FuncType get_imported_function(const module_name mn,
                                             const function_name fn) {
    auto* const module_handle = GetModuleHandleA(mn.data());
    if(!module_handle) {
        return nullptr;
    }
    return reinterpret_cast<FuncType>(GetProcAddress(module_handle, fn.data()));
}

template<typename FuncType>
bool hook_import(HMODULE module, module_name mn, function_name fn,
                 FuncType& original, FuncType hook) {
    // Store a pointer to the actual function; this function can be called
    // multiple times in the same address space so we only want to do this
    // the first time
    if(!original) {
        original = get_imported_function<FuncType>(mn, fn);
        if(!original) {
            return false;
        }
    }

    auto* import_descr = get_import_table(module);
    if(!import_descr) {
        return false;
    };
    // Walk the import descriptor table
    const auto base_addr = reinterpret_cast<uintptr_t>(module);
    for(; import_descr->Name != 0; std::advance(import_descr, 1)) {
        // Skip other modules
        if(const auto* const name =
               reinterpret_cast<LPCSTR>(base_addr + import_descr->Name);
           name == mn) {
            continue;
        }

        auto* thunk = reinterpret_cast<PIMAGE_THUNK_DATA>(
            base_addr + import_descr->FirstThunk);
        auto* original_thunk = reinterpret_cast<PIMAGE_THUNK_DATA>(
            base_addr + import_descr->OriginalFirstThunk);
        for(; thunk->u1.Function != 0;
            std::advance(thunk, 1), std::advance(original_thunk, 1)) {
            // Skip other functions
            if(thunk->u1.Function != reinterpret_cast<uintptr_t>(original)) {
                continue;
            }

            // Patch the function pointer
            DWORD old_protection = 0;
            VirtualProtect(thunk, sizeof(PIMAGE_THUNK_DATA), PAGE_READWRITE,
                           &old_protection);
            thunk->u1.Function = reinterpret_cast<uintptr_t>(hook);
            VirtualProtect(thunk, sizeof(PIMAGE_THUNK_DATA), old_protection,
                           &old_protection);
        }
    }
    return true;
}
}; // namespace iat_hook

#endif
