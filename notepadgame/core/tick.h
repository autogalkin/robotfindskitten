#pragma once


#include <chrono>
#include "boost/signals2.hpp"

class ticker
{
public:
    
    explicit ticker()
    {
        
        using namespace std::chrono_literals;
        lag_accumulator_ = 0ns;
    }
    virtual ~ticker() = default;
    
    ticker(ticker &other) = delete;
    ticker& operator=(const ticker& other) = delete;
    ticker(ticker&& other) noexcept = delete;
    ticker& operator=(ticker&& other) noexcept = delete;
    
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
        last_time = clock::now();
        
    }
    
    virtual void tickframe()
    {
        using namespace std::literals;
        
        const time_point new_t = clock::now();
        
        auto frametime = new_t - last_time;
        
        /*250ms is the limit put in place on the frame time to cope with the spiral of death.
         *It doesn't have to be 250ms exactly but it should be sufficiently high enough to deal with (hopefully temporary) spikes in load.*/
        if (frametime > 250ms)
            frametime = 250ms;
        
        last_time = new_t;
        
        lag_accumulator_ += dt - frametime;
        
        while(lag_accumulator_ > 0ms)
        {
            const auto sleep_start_time = clock::now();
            std::this_thread::sleep_until(sleep_start_time + lag_accumulator_);
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
    time_point last_time = clock::now();
    // compute fps
    int frame_rate = 0;
    int frame_count = 0;
    std::chrono::time_point<std::chrono::steady_clock, std::chrono::seconds> seconds_tstep = std::chrono::time_point_cast<std::chrono::seconds>(std::chrono::steady_clock::now());
    
};



class tickable
{
public:
    explicit tickable();
    virtual ~tickable() = default;
protected: // prevent slicing
    tickable(const tickable& other)            noexcept : tickable() {} // connect to new tick
    tickable& operator=(const tickable& other) noexcept  {return *this;}
    tickable(tickable&& other)                 noexcept : tickable() {other.tick_connection.disconnect();}
    tickable& operator=(tickable&& other)      noexcept  {other.tick_connection.disconnect(); return *this;}
    
    virtual void tick() = 0;
    
private:
    boost::signals2::scoped_connection tick_connection;
};

struct C {
   
};

// waits for the end time and call a given function
class timer : public tickable
{
public:
    
    template<typename T, typename ...Args>
    requires std::invocable<T, Args...>
    explicit timer(T&& what_do, const std::chrono::milliseconds rate, const bool looped=false) : do_(std::forward<T>(what_do)),
        rate_(rate), looping_(looped) {}
    
    timer(const timer& other)                 = default;
    timer(timer&& other) noexcept             = default;
    timer& operator=(const timer& other)      = default;
    timer& operator=(timer&& other) noexcept  = default;
    ~timer() override                         = default;
    
    virtual void tick() override;
    void start() noexcept ;
    [[nodiscard]] bool is_loopping() const noexcept {return looping_;}
    [[nodiscard]] bool is_started()  const noexcept {return started_;}
    [[nodiscard]] bool is_finished() const noexcept {return stopped_;}
    void invalidate() noexcept {stopped_ = true;}
    void restart() noexcept {stopped_ = false; started_ = false; start();}
    [[nodiscard]] const ticker::time_point& get_start_time() const {return start_time_;}
    [[nodiscard]] const ticker::time_point& get_last_time()  const {return last_time_;}
    [[nodiscard]] const ticker::time_point& get_stop_time()  const {return stop_time_;}
protected:
    void update_last_time() noexcept {last_time_ = ticker::clock::now();}
    void call_do() const noexcept {do_();}
private:
    std::function<void()> do_;
    std::chrono::milliseconds rate_;
    ticker::time_point start_time_;
    ticker::time_point last_time_;
    ticker::time_point stop_time_;
    bool started_ = false;
    bool stopped_ = false;
    bool looping_  = false;
};

// calls a given function every tick to the end time
class timeline final : public timer
{
public:
    enum class direction : int8_t
    {
        forward = 1,
        reverse = -1
    };
    static direction invert(const direction d) { return static_cast<direction>(static_cast<int8_t>(d) * -1);}
    
    template<typename T, typename ...Args>
    requires std::invocable<T, Args...>
    timeline(T&& what_do, const std::chrono::milliseconds rate, const bool looped=false, const timeline::direction start_direction=direction::forward)
    : timer(std::forward<T>(what_do), rate, looped), direction_(start_direction){}
    
    timeline(const timeline& other)                 = default;
    timeline(timeline&& other) noexcept             = default;
    timeline& operator=(const timeline& other)      = default;
    timeline& operator=(timeline&& other) noexcept  = default;
    ~timeline() override                            = default;
    void tick() override;
    [[nodiscard]] direction get_current_direction() const noexcept {return direction_;}
private:
    
    direction direction_ = direction::forward;
};