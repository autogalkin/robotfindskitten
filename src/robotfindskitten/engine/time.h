#pragma once
#include <__msvc_chrono.hpp>
#include <chrono>
#include <cstdint>
#include <type_traits>
#include <iostream>

namespace timings {
using namespace std::chrono_literals;
// most suitable for measuring intervals
using clock = std::chrono::steady_clock;
static auto constexpr dt = std::chrono::duration<int64_t, std::ratio<1, 60>>{1};
using duration = decltype(clock::duration{} + dt);
using point = std::chrono::time_point<clock, duration>;

class fixed_time_step {
    duration lag_accum_ = 0ms;
    point prev_point_ = clock::now();

public:
    void sleep() {
        const point new_point = clock::now();
        auto frame_time = prev_point_ + dt - new_point;
        /*250ms is the limit put in place on the frame time to cope with the
         *spiral of death.*/
        static constexpr auto LIMIT = 250ms;
        if(frame_time > LIMIT) {
            frame_time = LIMIT;
        }
        lag_accum_ += frame_time;
        while(lag_accum_ > 0ms) {
            const auto sleep_start = clock::now();
            std::this_thread::sleep_for(lag_accum_);
            const auto sleep_end = clock::now();
            lag_accum_ -= sleep_end - sleep_start;
        }
        prev_point_ = clock::now();
    }
};

class fps_count {
    point start_point_ = clock::now();
    uint32_t frames_ = 0;

public:
    template<typename Printer>
        requires std::is_invocable_v<Printer, uint64_t>
    void fps(Printer&& printer) {
        frames_++;
        const point new_point_ = clock::now();
        const auto delta = new_point_ - start_point_;
        if(delta >= 1s) {
            printer(
                std::round(static_cast<double>(frames_)
                           / (std::chrono::duration<double>(delta).count())));
            frames_ = 0;
            start_point_ = new_point_;
        }
    }
};

} // namespace timings
