#pragma once
#include <Windows.h>
#include <memory>
#include <thread>
#include "boost/signals2.hpp"

#include "../input.h"
#include "../world.h"


class notepad final // notepad.exe wrapper
{
public:

    static notepad& get()
    {
        static notepad notepad{};
        return notepad;
    }
    enum options: uint8_t
    {
          empty                 =0x0
        , kill_focus            =0x1 << 0
        , hide_selection        =0x1 << 1
        , disable_mouse         =0x1 << 2
        
    };
    
    void connect_to_notepad(const HMODULE module /* notepad.exe module*/, const uint8_t start_options=options::empty)
    {
        start_options_ = start_options;
        hook_CreateWindowExW(module);
        hook_SendMessageW(module);
        hook_GetMessageW(module);
        hook_SetWindowTextW(module);
        
    }
    
    notepad(notepad &other) = delete;
    notepad& operator=(const notepad& other) = delete;
    notepad(notepad&& other) noexcept = delete;
    notepad& operator=(notepad&& other) noexcept = delete;
    
    [[nodiscard]] boost::signals2::signal<void ()>& get_on_open() {return on_open_;}
    [[nodiscard]] boost::signals2::signal<void ()>& get_on_close() {return on_close_;}
    [[nodiscard]] HWND get_main_window() const {return main_window_;}
    [[nodiscard]] const std::unique_ptr<input>& get_input_manager() const  {return input_;}
    [[nodiscard]] const std::unique_ptr<world>& get_world() const { return world_;}

protected:
    
    explicit notepad() : world_(nullptr), input_(nullptr), main_window_(), original_proc_(0) {}
    virtual ~notepad() = default;

    // custom WindowProc
    static LRESULT CALLBACK hook_wnd_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
    
    // Capture and block keyboard and mouse inputs
    bool hook_GetMessageW(HMODULE module) const;
    
    // Capture opened files
    bool hook_SendMessageW(const HMODULE module) const;

    // Capture window handles
    bool hook_CreateWindowExW(HMODULE module) const;

    // Block window title updates
    bool hook_SetWindowTextW(HMODULE module) const;

    void init(const HWND& main_window); // calls in hook_CreateWindowExW
    void init_world(HWND& edit_window) {world_ = std::make_unique<world>(edit_window);}
    // calls after the first catch hook_GetMessageW
    static DWORD WINAPI post_init_notepad_window(LPVOID);
    void close();
    
private:

    uint8_t start_options_{ options::empty };
    boost::signals2::signal<void ()> on_open_;
    boost::signals2::signal<void ()> on_close_;
    
    std::unique_ptr<world> world_{};
    std::unique_ptr<input> input_{};

    HWND main_window_;
    
    LONG_PTR original_proc_; // notepad.exe window proc 
};
