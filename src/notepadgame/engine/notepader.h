#pragma once
#pragma warning(push, 0)
#include <Windows.h>
#include <memory>
#include <thread>
#include "boost/signals2.hpp"
#pragma warning(pop)
#include "tick.h"
#include "input.h"
#include "engine.h"



// notepad.exe wrapper
class notepader final : public ticker
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
        , show_eol              =0x1 << 3
        , show_spaces           =0x1 << 4
        
    };
    
    void connect_to_notepad(const HMODULE module /* notepad.exe module*/, const uint8_t start_options=options::empty)
    {
        options_ = start_options;
        hook_CreateWindowExW(module);
        hook_SendMessageW(module);
        hook_GetMessageW(module);
        hook_SetWindowTextW(module);
        
    }
    
    [[nodiscard]] boost::signals2::signal<void ()>& get_on_open()             { return on_open_;  }
    [[nodiscard]] boost::signals2::signal<void ()>& get_on_close()            { return on_close_; }
    [[nodiscard]] HWND get_main_window() const                                { return main_window_;}
    [[nodiscard]] const std::unique_ptr<input>& get_input_manager() const     { return input_; }
    [[nodiscard]] const std::unique_ptr<engine>& get_engine() const           { return engine_; }
    
    void set_window_title(const std::wstring_view title) const { SetWindowTextW(main_window_, title.data()); }
    
    
protected:
    
    explicit notepader() : engine_(nullptr), input_(nullptr),  main_window_(), original_proc_(0)
    {
    }

    virtual ~notepader() override = default;
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

    virtual void tickframe() override;

private:

    uint8_t options_{ options::empty };
    boost::signals2::signal<void ()> on_open_{};
    boost::signals2::signal<void ()> on_close_{};
    
    std::unique_ptr<engine> engine_{};
    std::unique_ptr<input> input_{};
    HWND main_window_;
    LONG_PTR original_proc_; // notepad.exe window proc 
};

