/**
 * @file
 * @brief Managing game time
 */

#pragma once
#include <chrono>
#include <cstdint>
#include <type_traits>

namespace timings {
using namespace std::chrono_literals;

// most suitable for measuring intervals
using clock = std::chrono::steady_clock;

// fixed time stem inderval
static auto constexpr dt = std::chrono::duration<int64_t, std::ratio<1, 60>>{1};

using duration = decltype(clock::duration{} + dt);
// clock time point
using point = std::chrono::time_point<clock, duration>;



/**
 * @class fixed_time_step
 * @brief A class is responsible for maintaining fixed time stem game loop
 *
 */
class fixed_time_step {
    duration lag_accum_ = 0ms;
    point prev_point_ = clock::now();

public:
    /**
     * @brief Sleep between current frame end and next frame start
     *
     * Usage: call in while loop
     */
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

/**
 * @class fps_count
 * @brief Count FPS and return count of frames between calls fps()
 */
class fps_count {
    point start_point_ = clock::now();
    uint32_t frames_ = 0;

public:
    /**
     * @brief Count fps between calls
     *
     * @tparam Printer Predicate to accept the result if it is ready
     * @param printer a lambda of type Printer
     */
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
