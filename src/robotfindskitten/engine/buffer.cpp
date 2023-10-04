#include "engine/buffer.h"

#include "engine/details/base_types.hpp"



template<typename Buf>
[[nodiscard]] auto& at(Buf& buf_2d, size_t buf_row_width,
                       pos char_on_screen) {
    return buf_2d[char_on_screen.y * buf_row_width + char_on_screen.x];
}

void back_buffer::draw(pos pivot, sprite_view sh, int32_t depth) {
    traverse_sprite_positions(pivot, sh,
                              [this, depth](const pos& p, const char_size ch) {
                                  auto& z_value = at(z_buf_, width_, p);
                                  if(depth >= z_value) {
                                      z_value = depth;
                                      at(buf_, width_, p) = ch;
                                  }
                              });
}

void back_buffer::erase(pos pivot, sprite_view sh, int32_t depth) {
    traverse_sprite_positions(pivot, sh,
                              [this, depth](pos p, char_size) {
                                  auto& z_value = at(z_buf_, width_, p);
                                  if(depth >= z_value) {
                                      z_value = 0;
                                      at(buf_, width_, p) = sprite::whitespace;
                                  }
                              });
}
