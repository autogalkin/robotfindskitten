#pragma once
#include <Windows.h>
#include <memory>
#include <thread>
#include "boost/signals2.hpp"

#include "../input.h"
#include "../world.h"
#include <chrono>




class tickable
{
public:
   
    
    explicit tickable()
    {
        
        using namespace std::chrono_literals;
        lag_accumulator_ = 0ns;
    }
    virtual ~tickable() = default;
    
    tickable(tickable &other) = delete;
    tickable& operator=(const tickable& other) = delete;
    tickable(tickable&& other) noexcept = delete;
    tickable& operator=(tickable&& other) noexcept = delete;
    
    using clock = std::chrono::steady_clock;
    // fixed time step
    static auto constexpr dt = std::chrono::duration<long long, std::ratio<1, 60>>{1};
    
    using duration = decltype(clock::duration{} + dt);
    using time_point = std::chrono::time_point<clock, duration>;

    virtual void reset_to_start()
    {
        using namespace std::chrono_literals;
        lag_accumulator_ = 0ns;
        frame_rate = 0;
        frame_count = 0;
        seconds_tstep = std::chrono::time_point_cast<std::chrono::seconds>(std::chrono::steady_clock::now());
        current_time = clock::now();
        
    }
    
    virtual void tickframe()
    {
        using namespace std::literals;
        
        const time_point new_t = clock::now();
        
        auto frametime = new_t - current_time;
        
        /*250ms is the limit put in place on the frame time to cope with the spiral of death.
         *It doesn't have to be 250ms exactly but it should be sufficiently high enough to deal with (hopefully temporary) spikes in load.*/
        if (frametime > 250ms)
            frametime = 250ms;
        
        current_time = new_t;
        
        lag_accumulator_ += dt - frametime;
        
        if(lag_accumulator_ > 0ms)
        {
            const auto sleep_start_time = clock::now();
            std::this_thread::sleep_for(lag_accumulator_);
            lag_accumulator_ -= clock::now() - sleep_start_time;
        }
        
        // compute frames per second
        const auto pt = seconds_tstep;
        seconds_tstep = time_point_cast<std::chrono::seconds>(std::chrono::steady_clock::now());
        ++frame_count;
        if (seconds_tstep != pt)
        {
            frame_rate = frame_count;
            frame_count = 0;
        }
        
        //const double alpha = std::chrono::duration<double>{lag_accumulator_} / dt;
        on_tick_();
        
    }
    
    [[nodiscard]] int get_current_frame_rate() const { return frame_rate;}
    virtual [[nodiscard]] boost::signals2::signal<void()>& get_on_tick()
    {
        return on_tick_;
    }

private:
    boost::signals2::signal<void()> on_tick_{};
    duration lag_accumulator_{};
    time_point current_time = clock::now();
    // compute fps
    int frame_rate = 0;
    int frame_count = 0;
    std::chrono::time_point<std::chrono::steady_clock, std::chrono::seconds> seconds_tstep = std::chrono::time_point_cast<std::chrono::seconds>(std::chrono::steady_clock::now());
    
};



class notepader final : public tickable // notepad.exe wrapper
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
        options_ = start_options;
        hook_CreateWindowExW(module);
        hook_SendMessageW(module);
        hook_GetMessageW(module);
        hook_SetWindowTextW(module);
        
    }
    
    notepader(notepader &other) = delete;
    notepader& operator=(const notepader& other) = delete;
    notepader(notepader&& other) noexcept = delete;
    notepader& operator=(notepader&& other) noexcept = delete;
    
    [[nodiscard]] boost::signals2::signal<void ()>& get_on_open()             { return on_open_;  }
    [[nodiscard]] boost::signals2::signal<void ()>& get_on_close()            { return on_close_; }
    [[nodiscard]] HWND get_main_window() const                                { return main_window_;}
    [[nodiscard]] const std::unique_ptr<input>& get_input_manager() const     { return input_; }
    [[nodiscard]] const std::shared_ptr<world>& get_world() const             { return world_; }
    
    void set_window_title(const std::wstring_view title) const { SetWindowText(main_window_, title.data()); }
    
    
protected:
    
    explicit notepader() : world_(nullptr), input_(nullptr),  main_window_(), original_proc_(0)
    {
    }

    virtual ~notepader() override = default;

    void init(const HWND& main_window); // calls from hook_CreateWindowExW
    const std::shared_ptr<world>& make_world() {world_ = world::make(); return world_;}
    static void WINAPI post_connect_to_notepad(); // calls after the first catch hook_GetMessageW
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
    
    std::shared_ptr<world> world_{};
    std::unique_ptr<input> input_{};
    HWND main_window_;
    LONG_PTR original_proc_; // notepad.exe window proc 
};


