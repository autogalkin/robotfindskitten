/**
 * @file
 * @brief Managing the game time.
 */

#ifndef _CPP_PROJECTS_ROBOTFINDSKITTEN_SRC_ROBOTFINDSKITTEN_ENGINE_TIME_H
#define _CPP_PROJECTS_ROBOTFINDSKITTEN_SRC_ROBOTFINDSKITTEN_ENGINE_TIME_H

#include <chrono>
#include <cstdint>
#include <type_traits>

namespace timings {
using namespace std::chrono_literals;

// Most suitable for measuring intervals
using clock = std::chrono::steady_clock;

// The fixed time stem inderval
constexpr auto dt = std::chrono::duration<int64_t, std::ratio<1, 60>>{1};

using duration = decltype(clock::duration{} + dt);
// A clock time point
using point = std::chrono::time_point<clock, duration>;

/**
 * @brief A class is responsible for maintaining fixed time stem game loop
 */
class fixed_time_step {
    duration lag_accum_ = 0ms;
    point prev_point_ = clock::now();

public:
    /**
     * @brief Sleep between the end of the current frame and the start of the
     * next frame, and return the duration of the frame from the previous
     * frame's start to the new frame's start because it can be not equal to dt
     * exactly. Usage: call in the while loop
     */
    duration sleep() {
        const point new_point = clock::now();
        auto frame_time = new_point - prev_point_;
        duration real_dt = frame_time;
        lag_accum_ += dt - frame_time;
        /*250ms is the limit put in place on the frame time to cope with the
         *spiral of death.*/
        static constexpr auto LIMIT = 250ms;
        if(lag_accum_ > LIMIT) {
            lag_accum_ = LIMIT;
        }
        while(lag_accum_ > 0ms) {
            const auto sleep_start = clock::now();
            std::this_thread::sleep_for(lag_accum_);
            const auto sleep_end = clock::now();
            lag_accum_ -= sleep_end - sleep_start;
            real_dt += sleep_end - sleep_start;
        }
        prev_point_ = clock::now();
        return real_dt;
    }
};

/**
 * @brief Counts FPS and return count of frames between calls the fps() function
 * in the while loop
 */
class fps_count {
public:
    using value_type = uint32_t;

private:
    point start_point_ = clock::now();
    value_type frames_ = 0;

public:
    /**
     * @brief Counts fps between calls
     * @tparam Printer A predicate to accept the result if it is ready
     * @param printer A lambda of type Printer
     */
    template<typename Printer>
        requires std::is_invocable_v<Printer, fps_count::value_type>
    void fps(Printer&& printer) {
        frames_++;
        const point new_point_ = clock::now();
        const auto delta = new_point_ - start_point_;
        if(delta >= 1s) {
            printer(
                std::lround(static_cast<double>(frames_)
                            / (std::chrono::duration<double>(delta).count())));
            frames_ = 0;
            start_point_ = new_point_;
        }
    }
};

} // namespace timings

#endif
