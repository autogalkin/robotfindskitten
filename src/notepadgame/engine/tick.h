#pragma once
#pragma warning(push, 0)
#include "boost/signals2.hpp"
#include <chrono>
#pragma warning(pop)
#include "details/nonconstructors.h"

namespace gametime {
using clock = std::chrono::steady_clock;
// fixed time step
static auto constexpr dt =
    std::chrono::duration<long long, std::ratio<1, 60>>{1};
using duration = decltype(clock::duration{} + dt);
using time_point = std::chrono::time_point<clock, duration>;

} // namespace gametime

class ticker : public noncopyable, public nonmoveable {
  public:
    explicit ticker() {

        using namespace std::chrono_literals;
        lag_accumulator_ = 0ns;
    }
    virtual ~ticker() = default;

    ticker(ticker& other) = delete;
    ticker& operator=(const ticker& other) = delete;
    ticker(ticker&& other) noexcept = delete;
    ticker& operator=(ticker&& other) noexcept = delete;

    virtual void reset_to_start() {
        using namespace std::chrono_literals;
        lag_accumulator_ = 0ns;
        frame_rate = 0;
        frame_count = 0;
        seconds_tstep = std::chrono::time_point_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now());
        last_time = gametime::clock::now();
    }

    virtual void tickframe() {
        using namespace std::literals;

        const gametime::time_point new_t = gametime::clock::now();

        auto frametime = new_t - last_time;

        /*250ms is the limit put in place on the frame time to cope with the
         *spiral of death. It doesn't have to be 250ms exactly but it should be
         *sufficiently high enough to deal with (hopefully temporary) spikes in
         *load.*/
        if (frametime > 250ms)
            frametime = 250ms;

        last_time = new_t;

        lag_accumulator_ += gametime::dt - frametime;

        while (lag_accumulator_ > 0ms) {
            const auto sleep_start_time = gametime::clock::now();
            std::this_thread::sleep_until(sleep_start_time + lag_accumulator_);
            auto nt = gametime::clock::now();
            lag_accumulator_ -= nt - sleep_start_time;
        }

        // compute frames per second
        const auto pt = seconds_tstep;
        seconds_tstep = time_point_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now());
        ++frame_count;
        if (seconds_tstep != pt) {
            frame_rate = frame_count;
            frame_count = 0;
        }

        // const double alpha = std::chrono::duration<double>{lag_accumulator_}
        // / dt;
        on_tick_(gametime::dt);
    }

    [[nodiscard]] int get_current_frame_rate() const { return frame_rate; }
    [[nodiscard]] virtual boost::signals2::signal<void(gametime::duration)>&
    get_on_tick() {
        return on_tick_;
    }

  private:
    boost::signals2::signal<void(gametime::duration)> on_tick_{};
    gametime::duration lag_accumulator_{};
    gametime::time_point last_time = gametime::clock::now();
    // compute fps
    int frame_rate = 0;
    int frame_count = 0;
    std::chrono::time_point<std::chrono::steady_clock, std::chrono::seconds>
        seconds_tstep = std::chrono::time_point_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now());
};

class tickable {
  public:
    explicit tickable();
    virtual ~tickable() = default;

  protected: // prevent slicing
    tickable(const tickable&) noexcept : tickable() {} // connect to new tick
    tickable& operator=(const tickable&) noexcept { return *this; }
    tickable(tickable&& other) noexcept : tickable() {
        other.tick_connection.disconnect();
    }
    tickable& operator=(tickable&& other) noexcept {
        other.tick_connection.disconnect();
        return *this;
    }

    virtual void tick(gametime::duration delta) = 0;

  private:
    boost::signals2::scoped_connection tick_connection;
};
