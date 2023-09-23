#pragma once
#include <chrono>
#include "details/nonconstructors.h"

namespace time {
using namespace std::chrono_literals;
// most suitable for measuring intervals
using clock = std::chrono::steady_clock;
static auto constexpr dt =
    std::chrono::duration<long long, std::ratio<1, 60>>{1};
using duration = decltype(clock::duration{} + dt);
using point = std::chrono::time_point<clock, duration>;

class fixed_time_step {
    time::duration lag_accum_ = 0ms;
    time::point prev_point_ = time::clock::now();

  public:
    void sleep() {
        const time::point new_point = time::clock::now();
        /*250ms is the limit put in place on the frame time to cope with the
         *spiral of death.*/
        const auto frame_time = std::min<std::common_type_t<
            decltype(prev_point_ + time::dt - new_point), decltype(250ms)>>(
            new_point - prev_point_ + time::dt, 250ms);

        lag_accum_ += frame_time;

        if (lag_accum_ > 0ms) {
            const auto sleep_start = time::clock::now();
            std::this_thread::sleep_for(lag_accum_);
            const auto sleep_end = time::clock::now();
            lag_accum_ -= sleep_end - sleep_start;
        }
        prev_point_ = time::clock::now();
    }
};

class fps_count {
    time::point start_point_ = time::clock::now();
    time::duration frames_ = 0ms;

  public:
    uint64_t fps() {
        ++frames_;
        const time::point new_point_ = time::clock::now();
        const auto delta = new_point_ - start_point_;
        if (delta > 1s) {
            const auto fps = frames_ / delta;
            frames_ = 0ms;
            start_point_ = time::clock::now();
            return fps;
        }
        return 0;
    }
};
/*
using signal_t = boost::signals2::signal<void(time::duration)>;
// signal_t on_tick{};

class tickable_base : noncopyable, nonmoveable {
  public:
    virtual ~tickable_base() = default;

    explicit tickable_base(time::signal_t& on_tick)
        : tick_connection(on_tick.connect(
              [this](const time::duration delta) { tick(delta); })) {}

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

    virtual void tick(time::duration delta) = 0;

  private:
    boost::signals2::scoped_connection tick_connection;
};
  //
*/


} // namespace time
