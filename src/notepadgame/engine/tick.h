#pragma once
#pragma warning(push, 0)
#include "boost/signals2.hpp"
#include <chrono>
#pragma warning(pop)
#include "details/nonconstructors.h"

namespace gametime {
// most suitable for measuring intervals
using clock = std::chrono::steady_clock;
// fixed time step
static auto constexpr dt =
    std::chrono::duration<long long, std::ratio<1, 60>>{1};
using duration = decltype(clock::duration{} + dt);
using time_point = std::chrono::time_point<clock, duration>;

} // namespace gametime

class ticker final : public noncopyable, public nonmoveable {
  public:
    explicit ticker() {
        using namespace std::chrono_literals;
        lag_accumulator_ = 0ns;
    }
    using signal_t = boost::signals2::signal<void(gametime::duration)>;
    signal_t on_tick{};

    virtual void reset_to_start() {
        using namespace std::chrono_literals;
        lag_accumulator_ = 0ns;
        frame_rate = 0;
        frame_count = 0;
        seconds_tstep = std::chrono::time_point_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now());
        last_time = gametime::clock::now();
    }

    virtual void tick_frame() {
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
        on_tick(gametime::dt);
    }

    [[nodiscard]] int get_current_frame_rate() const { return frame_rate; }

  private:
    gametime::duration lag_accumulator_{};
    gametime::time_point last_time = gametime::clock::now();
    // compute fps
    int frame_rate = 0;
    int frame_count = 0;
    std::chrono::time_point<std::chrono::steady_clock, std::chrono::seconds>
        seconds_tstep = std::chrono::time_point_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now());
};

class tickable_base : noncopyable, nonmoveable {
  public:
    virtual ~tickable_base() = default;

    explicit tickable_base(ticker::signal_t& on_tick)
        : tick_connection(on_tick.connect(
              [this](const gametime::duration delta) { tick(delta); })) {}

  protected: // prevent slicing
    // tickable_base(const tickable_base& rhs) noexcept : tickable_base() {} //
    // connect to new tick tickable_base& operator=(const tickable_base&)
    // noexcept { return *this; } tickable_base(tickable_base&& other) noexcept
    // : tickable_base() {
    //     other.tick_connection.disconnect();
    // }
    // tickable_base& operator=(tickable_base&& other) noexcept {
    //    other.tick_connection.disconnect();
    //    return *this;
    // }

    virtual void tick(gametime::duration delta) = 0;

  private:
    boost::signals2::scoped_connection tick_connection;
};
