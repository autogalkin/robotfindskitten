#include "engine/buffer.h"
#include "details/nonconstructors.h"
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

template <typename Buf>
[[nodiscard]] auto& at(Buf& buf_2d, const size_t buf_row_width,
                       const pos char_on_screen) {
    return buf_2d[char_on_screen.y * buf_row_width + char_on_screen.x];
}

void back_buffer::draw(const pos& pivot, const sprite_view sh, int32_t depth) {

    std::unique_lock<std::shared_mutex> lock(rw_lock_);
    traverse_sprite_positions(pivot, sh,
                              [this, depth](const pos& p, const char_size ch) {
                                  auto& z_value = at(z_buf_, width_, p);
                                  if (depth >= z_value) {
                                      z_value = depth;
                                      at(buf_, width_, p) = ch;
                                  }
                              });
}

void back_buffer::erase(const pos& pivot, const sprite_view sh, int32_t depth) {
    std::unique_lock<std::shared_mutex> lock(rw_lock_);
    traverse_sprite_positions(pivot, sh,
                              [this, depth](const pos& p, char_size) {
                                  auto& z_value = at(z_buf_, width_, p);
                                  if (depth >= z_value) {
                                      z_value = 0;
                                      at(buf_, width_, p) = sprite::whitespace;
                                  }
                              });
}
