/**
 * @file
 * @brief Render into BackBuffer
 */

#pragma once
#ifndef _CPP_PROJECTS_ROBOTFINDSKITTEN_SRC_ROBOTFINDSKITTEN_ENGINE_BUFFER_H
#define _CPP_PROJECTS_ROBOTFINDSKITTEN_SRC_ROBOTFINDSKITTEN_ENGINE_BUFFER_H

#include <algorithm>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

#include "engine/details/base_types.hpp"

template<typename Mutex>
class mutex_debug_wrapper: public Mutex {
public:
#ifndef NDEBUG
    void lock() {
        std::mutex::lock();
        m_holder = std::this_thread::get_id();
    }
#endif // #ifndef NDEBUG

#ifndef NDEBUG
    void unlock() {
        m_holder = std::thread::id();
        std::mutex::unlock();
    }
#endif // #ifndef NDEBUG

#ifndef NDEBUG
    bool try_lock() {
        if(std::mutex::try_lock()) {
            m_holder = std::thread::id();
            return true;
        }
        return false;
    }
#endif // #ifndef NDEBUG

#ifndef NDEBUG
    /**
     * @return true iff the mutex is locked by the caller of this method. */
    bool locked_by_caller() const {
        return m_holder == std::this_thread::get_id();
    }
#endif // #ifndef NDEBUG

private:
#ifndef NDEBUG
    std::atomic<std::thread::id> m_holder = std::thread::id{};
#endif // #ifndef NDEBUG
};

template<typename Mutex>
class thread_safe_trait {
    mutex_debug_wrapper<Mutex> mutex_;

public:
    std::lock_guard<mutex_debug_wrapper<Mutex>> lock() {
        return std::lock_guard<mutex_debug_wrapper<Mutex>>{mutex_};
    }
#ifndef NDEBUG
    bool is_locked_by_caller() {
        return mutex_.locked_by_caller();
    }
#endif // #ifndef NDEBUG
};
class not_thread_safe_trait {
public:
    int lock() {
        return 0;
    }
#ifndef NDEBUG
    bool is_locked_by_caller() {
        return true;
    }
#endif // #ifndef NDEBUG
};

/**
 * @class back_buffer
 * @brief An array of chars on the screen
 */
template<typename Trait = not_thread_safe_trait>
class back_buffer: public Trait {
public:
    using char_size = ::char_size;

private:
    size_t width_;
    // ensure null-terminated
    std::basic_string<char_size> buf_;
    std::vector<int32_t> z_buf_;
    void set_lines() {
        static constexpr int ENDL_SIZE = 1;
        for(npi_t i = 1; i < buf_.size() / width_; i++) {
            buf_[i * width_ - ENDL_SIZE] = '\n';
        }
    }

public:
    explicit back_buffer(pos extends)
        : width_(extends.x),
          buf_(std::basic_string<char_size>(
              static_cast<size_t>(extends.x * extends.y), ' ')),
          z_buf_(static_cast<size_t>(extends.x * extends.y)) {
        set_lines();
    }
    /**
     * @brief Draw a sprite into the buffer
     *
     * @param pivot a sprite start position
     * @param sh a sprite to render
     * @param depth a sprite z_depth
     */
    void draw(pos pivot, sprite_view sh, int32_t depth);
    /**
     * @brief Erase sprite from the byffer
     *
     * @param pivot a sprite start position
     * @param sh a sprite to erase
     * @param depth a sprite z_depth
     */
    void erase(pos pivot, sprite_view sh, int32_t depth);
    /**
     * @brief clear all buffer to initial state
     *
     * Keep spaces and \n
     */
    void clear() {
        assert(this->is_locked_by_caller());
        std::fill(buf_.begin(), buf_.end(), ' ');
        set_lines();
        std::fill(z_buf_.begin(), z_buf_.end(), 0);
    };

    /**
     * @brief View buffer content
     *
     * Thread-Safe read-only access to the buffer
     *
     * @tparam Visitor Predicate to accept
     * a buffer content void(std::basic_string<char_size>& str)
     * @param visitor Visitor instance
     */
    template<typename Visitor>
        requires std::is_invocable_v<Visitor,
                                     // Need for Scintilla api call, ensure \0
                                     const std::basic_string<char_size>&>
    void view(Visitor&& visitor) {
        const auto maybe_lock = this->lock();
        visitor(buf_);
    }

private:
    // visitor traverse all sprite matrix
    template<typename Visitor>
        requires std::is_invocable_v<Visitor, pos, char_size>
    void traverse_sprite_positions(pos sprite_pivot, sprite_view sp,
                                   Visitor&& visitor) const {
        for(size_t i = 0; i < sp.data().size(); i++) {
            if(pos p = sprite_pivot + pos(i % sp.width(), i / sp.width());
               p.x >= 0 && p.x < width_ - 1 && p.y >= 0
               && p.y < buf_.size() / (width_ - 1)) {
                visitor(p, sp.data()[i]);
            }
        }
    }
};

template<typename Buf>
[[nodiscard]] auto& at(Buf& buf_2d, size_t buf_row_width, pos char_on_screen) {
    return buf_2d[char_on_screen.y * buf_row_width + char_on_screen.x];
}

template<typename Trait>
void back_buffer<Trait>::draw(pos pivot, sprite_view sh, int32_t depth) {
    assert(this->is_locked_by_caller());
    traverse_sprite_positions(pivot, sh,
                              [this, depth](const pos& p, const char_size ch) {
                                  auto& z_value = at(z_buf_, width_, p);
                                  if(depth >= z_value) {
                                      z_value = depth;
                                      at(buf_, width_, p) = ch;
                                  }
                              });
}

template<typename Trait>
void back_buffer<Trait>::erase(pos pivot, sprite_view sh, int32_t depth) {
    assert(this->is_locked_by_caller());
    traverse_sprite_positions(pivot, sh, [this, depth](pos p, char_size) {
        auto& z_value = at(z_buf_, width_, p);
        if(depth >= z_value) {
            z_value = 0;
            at(buf_, width_, p) = sprite::whitespace;
        }
    });
}

#endif
