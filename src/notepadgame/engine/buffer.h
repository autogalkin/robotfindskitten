#pragma once
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>

#include <memory>
#include <vector>

#include "engine/details/base_types.hpp"
#include <algorithm>
#include <numeric>
#include "range/v3/view/enumerate.hpp"
#include "range/v3/view/filter.hpp"
#include <ranges>
#include "shared_mutex"
#include <glm/glm.hpp>

// the array of chars on the screen
class back_buffer {
    size_t width_;
    // ensure null-terminated
    std::basic_string<char_size> buf_;
    std::vector<int32_t> z_buf_;
    void set_lines() {
        for(npi_t i = 1; i < buf_.size() / width_; i++) {
            static constexpr int ENDL_SIZE = 1;
            buf_[i * width_ - ENDL_SIZE] = '\n';
        }
    }

public:
    mutable std::mutex mutex_;
    explicit back_buffer(pos size)
        : width_(size.x), buf_(std::basic_string<char_size>(
                              static_cast<size_t>(size.x * size.y), ' ')),
          z_buf_(static_cast<size_t>(size.x * size.y)) {
        set_lines();
    }
    void draw(const pos& pivot, sprite_view sh, int32_t depth);
    void erase(const pos& pivot, sprite_view sh, int32_t depth);
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        std::fill(buf_.begin(), buf_.end(), ' ');
        set_lines();
        std::fill(z_buf_.begin(), z_buf_.end(), 0);
    };

    template<typename Visitor>
        requires std::is_invocable_v<Visitor,
                                     const std::basic_string<char_size>&>
    void view(Visitor&& visitor) const {
        std::lock_guard<std::mutex> lock(mutex_);
        visitor(buf_);
    }

private:
    // visitor traverse all sprite matrix
    template<typename Visitor>
        requires std::is_invocable_v<Visitor, const pos, char_size>
    void traverse_sprite_positions(pos pivot, sprite_view sp,
                                   Visitor&& visitor) const;
};

template<typename Visitor>
    requires std::is_invocable_v<Visitor, const pos, char_size>
void back_buffer::traverse_sprite_positions(const pos sprite_pivot,
                                            const sprite_view sp,
                                            Visitor&& visitor) const {
    for(size_t i = 0; i < sp.data().size(); i++) {
        if(pos p = sprite_pivot + pos(i % sp.width(), i / sp.width());
           p.x >= 0 && p.x < width_ - 1 && p.y >= 0
           && p.y < buf_.size() / (width_ - 1)) {
            visitor(p, sp.data()[i]);
        }
    }
}
