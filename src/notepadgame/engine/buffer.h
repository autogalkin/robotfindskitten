#pragma once
#include <stdint.h>
#include <string_view>
#include <type_traits>

#include "df/dirtyflag.h"
#include <memory>
#include <vector>


#include "engine/details/base_types.h"
#include <algorithm>
#include <numeric>
#include "range/v3/view/enumerate.hpp"
#include "range/v3/view/filter.hpp"
#include <ranges>
#include "shared_mutex"


// the array of chars on the screen
class back_buffer {
    size_t width_;
    // ensure null-terminated
    std::basic_string<char_size> buf;
    mutable std::shared_mutex rw_lock_;

  public:
    explicit back_buffer(size_t width, size_t height)
        : width_(width), buf(std::basic_string<char_size>(width * height, ' ')),
          rw_lock_() {
        for (npi_t i = 1; i <= height; i++) {
            static constexpr int ENDL_SIZE = 1;
            buf[i * width - ENDL_SIZE] = '\n';
        }
    }
    void draw(const position_t& pivot, const shape::sprite& sh, int32_t depth);
    void erase(const position_t& pivot, const shape::sprite& sh, int32_t depth);

    template <typename Visitor>
        requires std::is_invocable_v<Visitor,
                                     const std::basic_string<char_size>&>
    void view(Visitor&& visitor) const {
        std::shared_lock<std::shared_mutex> lock(rw_lock_);
        visitor(buf);
    }

  private:
    [[nodiscard]] char_size& at(const position_t& char_on_screen);
    // visitor traverse all sprite matrix
    template <typename Visitor>
        requires std::is_invocable_v<Visitor, const position_t&, char_size>
    void traverse_sprite_positions(const position_t& pivot,
                                   const shape::sprite& sh,
                                   Visitor&& visitor) const;
};

template <typename Visitor>
    requires std::is_invocable_v<Visitor, const position_t&, char_size>
void back_buffer::traverse_sprite_positions(const position_t& pivot,
                                            const shape::sprite& sh,
                                            Visitor&& visitor) const {
    const position_t screen_pivot = pivot;

    for (auto rows = sh.data.rowwise();
         auto [line, row] : rows | ranges::views::enumerate) {
        for (int byte_i{-1};
             const auto part_of_sprite :
             row | ranges::views::filter([&byte_i](const char_size c) {
                 ++byte_i;
                 return c != shape::whitespace;
             })) {
            if (position_t p =
                    screen_pivot + position_t{static_cast<npi_t>(line), byte_i};
                p.index_in_line() >= 0 && p.index_in_line() <= width_ &&
                p.line() >= 0 && p.line() <= buf.size() / width_) {
                visitor(p, part_of_sprite);
            }
        }
    }
}
