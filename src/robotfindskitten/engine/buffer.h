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
class thread_safe_trait{
    Mutex mutex_;
public:
    std::lock_guard<Mutex> lock(){
        return std::lock_guard<Mutex> {mutex_};
    }
};
class not_thread_safe_trait{
    public:
    int lock(){
        return 0;
    }
};

/**
 * @class back_buffer
 * @brief An array of chars on the screen
 */
template<typename Trait=not_thread_safe_trait>
class back_buffer: public Trait {
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
        : width_(extends.x), buf_(std::basic_string<char_size>(
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
        auto maybe_lock = this->lock();
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
    void view(Visitor&& visitor) const {
        auto maybe_lock = this->lock();
        visitor(buf_);
    }

private:
    // visitor traverse all sprite matrix
    template<typename Visitor>
        requires std::is_invocable_v<Visitor, pos, char_size>
    void traverse_sprite_positions(pos pivot, sprite_view sp,
                                   Visitor&& visitor) const;

};

template<typename Visitor>
    requires std::is_invocable_v<Visitor, pos, char_size>
void back_buffer::traverse_sprite_positions(pos sprite_pivot, sprite_view sp,
                                            Visitor&& visitor) const {
    for(size_t i = 0; i < sp.data().size(); i++) {
        if(pos p = sprite_pivot + pos(i % sp.width(), i / sp.width());
           p.x >= 0 && p.x < width_ - 1 && p.y >= 0
           && p.y < buf_.size() / (width_ - 1)) {
            visitor(p, sp.data()[i]);
        }
    }
}


#endif
