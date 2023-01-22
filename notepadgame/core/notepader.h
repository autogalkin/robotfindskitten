#pragma once
#include <Windows.h>
#include <memory>
#include <thread>
#include "boost/signals2.hpp"

#include "../input.h"
#include "../world.h"


class notepader final // notepad.exe wrapper
{
public:

    static notepader& get()
    {
        static notepader notepad{};
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
    
    notepader(notepader &other) = delete;
    notepader& operator=(const notepader& other) = delete;
    notepader(notepader&& other) noexcept = delete;
    notepader& operator=(notepader&& other) noexcept = delete;
    
    [[nodiscard]] boost::signals2::signal<void ()>& get_on_open() {return on_open_;}
    [[nodiscard]] boost::signals2::signal<void ()>& get_on_close() {return on_close_;}
    [[nodiscard]] HWND get_main_window() const {return main_window_;}
    [[nodiscard]] const std::unique_ptr<input>& get_input_manager() const  {return input_;}
    [[nodiscard]] const std::shared_ptr<world>& get_world() const { return world_;}

protected:
    
    explicit notepader() : world_(nullptr), input_(nullptr),  main_window_(), original_proc_(0)
    {
    }

    virtual ~notepader() = default;

    void init(const HWND& main_window); // calls from hook_CreateWindowExW
    const std::shared_ptr<world>& init_world() {world_ = std::make_shared<world>(); return world_;}
    static DWORD WINAPI post_init_notepad_window(LPVOID); // calls after the first catch hook_GetMessageW
    void close();

    // custom WindowProc
    static LRESULT CALLBACK hook_wnd_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
   

    // Capture and block keyboard and mouse inputs
    bool hook_GetMessageW(HMODULE module) const;
    
    // Capture opened files
    bool hook_SendMessageW(HMODULE module) const;

    // Capture window handles
    bool hook_CreateWindowExW(HMODULE module) const;

    // Block window title updates
    bool hook_SetWindowTextW(HMODULE module) const;

    
    
private:

    uint8_t start_options_{ options::empty };
    boost::signals2::signal<void ()> on_open_{};
    boost::signals2::signal<void ()> on_close_{};
    
    std::shared_ptr<world> world_{};
    std::unique_ptr<input> input_{};
    HWND main_window_;
    LONG_PTR original_proc_; // notepad.exe window proc 
};

