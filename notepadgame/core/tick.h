#pragma once


#include <chrono>
#include "boost/signals2.hpp"


namespace gametime
{
    using clock = std::chrono::steady_clock;
    // fixed time step
    static auto constexpr dt = std::chrono::duration<long long, std::ratio<1, 60>>{1};
    using duration = decltype(clock::duration{} + dt);
    using time_point = std::chrono::time_point<clock, duration>;
    
}


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
    
    virtual void reset_to_start()
    {
        using namespace std::chrono_literals;
        lag_accumulator_ = 0ns;
        frame_rate = 0;
        frame_count = 0;
        seconds_tstep = std::chrono::time_point_cast<std::chrono::seconds>(std::chrono::steady_clock::now());
        last_time = gametime::clock::now();
        
    }
    
    virtual void tickframe()
    {
        using namespace std::literals;
        
        const gametime::time_point new_t = gametime::clock::now();
        
        auto frametime = new_t - last_time;
        
        /*250ms is the limit put in place on the frame time to cope with the spiral of death.
         *It doesn't have to be 250ms exactly but it should be sufficiently high enough to deal with (hopefully temporary) spikes in load.*/
        if (frametime > 250ms)
            frametime = 250ms;
        
        last_time = new_t;
        
        lag_accumulator_ += gametime::dt - frametime;
        
        while(lag_accumulator_ > 0ms)
        {
            const auto sleep_start_time = gametime::clock::now();
            std::this_thread::sleep_until(sleep_start_time + lag_accumulator_);
            auto nt = gametime::clock::now();
            lag_accumulator_ -= nt - sleep_start_time;
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
        on_tick_(gametime::dt);
        
    }
    
    [[nodiscard]] int get_current_frame_rate() const { return frame_rate;}
    virtual [[nodiscard]] boost::signals2::signal<void(gametime::duration)>& get_on_tick()
    {
        return on_tick_;
    }

private:
    boost::signals2::signal<void(gametime::duration)> on_tick_{};
    gametime::duration lag_accumulator_{};
    gametime::time_point last_time = gametime::clock::now();
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
    
    virtual void tick(gametime::duration delta) = 0;
    
private:
    boost::signals2::scoped_connection tick_connection;
};
// TODO asio timer or tick function? https://stackoverflow.com/questions/49769347/boostasiodeadline-timer-1ms-lags-after-some-time
/*
class asio_timer
{
public:
    asio_timer(boost::asio::io_context& io, const ticker::clock::duration i, std::function<void()> cb)
        : interval(i), callback(std::move(cb)), timer(io)
    { }
    void start()
    {
        timer.expires_from_now(interval);
        timer.async_wait([this](const boost::system::error_code ec) {
            if (!ec) {
                callback();
                // if loop -> run();
            }
        });
    }
private:
    
    ticker::clock::duration const interval; 
    std::function<void()> callback;
    boost::asio::high_resolution_timer timer;
boost::asio::io_service io;

  boost::asio::deadline_timer timer(io, boost::posix_time::seconds(5));

  // work_for_io_service() will be called 
  // when async operation (async_wait()) finishes  
  // note: Though the async_wait() immediately returns
  //       but the callback function will be called when time expires
  timer.async_wait(&work;_for_io_service);

  std::cout << " If we see this before the callback function, we know async_wait() returns immediately.\n This confirms async_wait() is non-blocking call!\n";

  // the callback function, work_for_io_service(), will be called 
  // from the thread where io.run() is running. 
  io.run(); https://www.bogotobogo.com/cplusplus/Boost/boost_AsynchIO_asio_tcpip_socket_server_client_timer_A.php
Библиотека asio гарантирует, что обработчики обратного вызова будут вызываться только из потоков, которые в данный момент вызывают io_service::run() . Поэтому, если функция io_service::run() не вызывается, обратный вызов для завершения асинхронного ожидания никогда не будет вызван.
};
Если выполнение приложения не должно быть заблокировано, run() следует вызывать в новом потоке, так как он естественным образом блокирует только текущий поток.
*/

// waits for the end time and call a given function
class timer : public tickable
{
public:
    
    template<typename T, typename ...Args>
    requires std::invocable<T, Args...>
    explicit timer(T&& what_do, const std::chrono::milliseconds rate, const bool looped=false) : do_(std::forward<T>(what_do)),
        rate_(rate), looping_(looped) {}
    
    timer(const timer& other)                 = delete;
    timer(timer&& other) noexcept             = default;
    timer& operator=(const timer& other)      = delete;
    timer& operator=(timer&& other) noexcept  = default;
    ~timer() override                         = default;
    
    virtual void tick(gametime::duration delta) override;
    void start() noexcept ;
    [[nodiscard]] boost::signals2::signal<void()>& get_on_finished() { return on_finish;}
    [[nodiscard]] bool is_loopping() const noexcept {return looping_;}
    [[nodiscard]] bool is_started()  const noexcept {return started_;}
    [[nodiscard]] bool is_finished() const noexcept {return stopped_;}
    void stop() noexcept {stopped_ = true; on_finish();}
    void restart() noexcept {stopped_ = false; started_ = false; start();}
    [[nodiscard]] const gametime::time_point& get_start_time() const {return start_time_;}
    [[nodiscard]] const gametime::time_point& get_last_time()  const {return last_time_;}
    [[nodiscard]] const gametime::time_point& get_stop_time()  const {return stop_time_;}
protected:
    void update_last_time() noexcept {last_time_ = gametime::clock::now();}
    void call_do() const noexcept {do_();}
private:
    std::function<void()> do_;
    boost::signals2::signal<void()> on_finish;
    std::chrono::milliseconds rate_;
    gametime::time_point start_time_;
    gametime::time_point last_time_;
    gametime::time_point stop_time_;
    bool started_ = false;
    bool stopped_ = false;
    bool looping_  = false;
};


