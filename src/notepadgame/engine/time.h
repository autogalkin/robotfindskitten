﻿#pragma once
#include <__msvc_chrono.hpp>
#include <chrono>
#include <stdint.h>
#include <type_traits>
#include <iostream>

namespace timings {
using namespace std::chrono_literals;
// most suitable for measuring intervals
using clock = std::chrono::steady_clock;
static auto constexpr dt =
    std::chrono::duration<long long, std::ratio<1, 60>>{1};
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
        if(frame_time > 250ms)
            frame_time = 250ms;
        lag_accum_ += frame_time;
        if (lag_accum_ > 0ms) {
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
    uint64_t frames_ = 0;

  public:
    template<typename Printer>
    requires std::is_invocable_v<Printer, double>
    void fps(Printer&& printer) {
        frames_++;
        const point new_point_ = clock::now();
        const auto  delta = new_point_ - start_point_;
        if (delta >= 1s) {
            printer(frames_ / std::chrono::duration_cast<std::chrono::seconds>(delta).count());
            frames_ = 0;
            start_point_ = new_point_;
        }
    }
};

} // namespace timings
