#include "buffer.h"
#include "details/nonconstructors.h"
#include <stdint.h>
#include <string_view>
#include <type_traits>
#pragma warning(push, 0)
#include "df/dirtyflag.h"
#include <memory>
#include <vector>
#pragma warning(pop)
#include "details/base_types.h"
#include "ecs_processor_base.h"
#include "tick.h"

#include "details/base_types.h"
#include "details/gamelog.h"
#include "df/dirtyflag.h"
#include <algorithm>
#include <numeric>
#include "range/v3/view/enumerate.hpp"
#include "range/v3/view/filter.hpp"
// #include "notepad.h"
// #include "engine.h"
// #include "world.h"
#include <ranges>

bool back_buffer::is_in_buffer(
    const position_t& global_position) const noexcept {
    return global_position.index_in_line() < width_ &&
           global_position.line() < buf.size() / width_;
}

void back_buffer::draw(const position_t& pivot, const shape::sprite& sh,
                       int32_t depth) {

    std::unique_lock<std::shared_mutex> lock(rw_lock_);
    traverse_sprite_positions(
        pivot, sh, [this, depth](const position_t& p, const char_size ch) {
            // int32_t z_index =
            // p.line() * static_cast<int>(engine_->get_window_width()) +
            // p.index_in_line();
            // if (depth >= this->z_buffer_[z_index]) {
            // this->z_buffer_[z_index] = depth;
            at(p) = ch;
            //}
        });
}

void back_buffer::erase(const position_t& pivot, const shape::sprite& sh,
                        int32_t depth) {
    std::unique_lock<std::shared_mutex> lock(rw_lock_);
    traverse_sprite_positions(pivot, sh,
                              [this, depth](const position_t& p, char_size) {
                                  // int32_t z_index =
                                  //     p.line() *
                                  //     static_cast<int>(engine_->get_window_width())
                                  //     + p.index_in_line();
                                  // if (this->z_buffer_[z_index] <= depth) {
                                  at(p) = shape::whitespace;
                                  //}
                              });
}
char_size& back_buffer::at(const position_t& char_on_screen) {
    return buf[char_on_screen.line() * width_ + char_on_screen.index_in_line()];
}
