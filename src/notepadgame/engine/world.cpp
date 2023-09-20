
#include "details/base_types.h"
#include "df/dirtyflag.h"
#include <numeric>
#pragma warning(push, 0)
#include "range/v3/view/enumerate.hpp"
#include "range/v3/view/filter.hpp"
#include "utf8cpp/utf8.h"
#pragma warning(pop)
#include "engine.h"
#include "notepad.h"
#include "world.h"
#include <ranges>

// name for a \0 in buffer math operations
static constexpr int endl = 1;
// \n(changed to \0 if not need a \n) and \0 for a line in the buffer
static constexpr int special_chars_count = 2;

backbuffer::backbuffer(engine_t *owner) : engine_(owner) {
  scroll_changed_connection_ = engine_->get_on_scroll_changed().connect(
      [this](const position_t &new_scroll) {
        scroll_.pin() = new_scroll;
        // TODO dirty all actors after resize
        backbuffer::init(static_cast<int>(engine_->get_window_width()),
                         static_cast<int>(engine_->get_lines_on_screen()));
      });

  size_changed_connection = engine_->get_on_resize().connect(
      [this](const uint32_t width, const uint32_t height) {
        backbuffer::init(width, height / engine_->get_line_height());
      });
}

position_t backbuffer::global_position_to_buffer_position(
    const position_t &position) const noexcept {
  return {position.line() - scroll_.get().line(),
          position.index_in_line() - scroll_.get().index_in_line()};
}

bool backbuffer::is_in_buffer(const position_t &global_position) const noexcept {
  return global_position.line() >= scroll_.get().line() &&
         global_position.index_in_line() >= scroll_.get().index_in_line() &&
         global_position.index_in_line() <=
             static_cast<int>(engine_->get_window_width()) /
                     engine_->get_char_width() +
                 scroll_.get().index_in_line() &&
         global_position.line() <=
             engine_->get_lines_on_screen() + scroll_.get().line();
}

void backbuffer::draw(const position_t &pivot, const shape::sprite &sh,
                      int32_t depth) {

  traverse_sprite_positions(
      pivot, sh, [this, depth](const position_t &p, const char_size ch) {
        int32_t z_index =
            p.line() * static_cast<int>(engine_->get_window_width()) +
            p.index_in_line();
        if (depth >= this->z_buffer_[z_index]) {
          this->z_buffer_[z_index] = depth;
          at(p) = ch;
        }
      });
}

void backbuffer::erase(const position_t &pivot, const shape::sprite &sh,
                       int32_t depth) {
  traverse_sprite_positions(
      pivot, sh, [this, depth](const position_t &p, char_size) {
        int32_t z_index =
            p.line() * static_cast<int>(engine_->get_window_width()) +
            p.index_in_line();
        if (this->z_buffer_[z_index] <= depth) {
          at(p) = shape::whitespace;
        }
      });
}

void backbuffer::traverse_sprite_positions(
    const position_t &pivot, const shape::sprite &sh,
    const std::function<void(const position_t &, char_size)> &visitor) const {
  const position_t screen_pivot = global_position_to_buffer_position(pivot);

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
          p.index_in_line() >= 0 &&
          p.index_in_line() <= static_cast<int>(engine_->get_window_width()) /
                                   engine_->get_char_width() &&
          p.line() >= 0 && p.line() <= engine_->get_lines_on_screen()) {
        visitor(p, part_of_sprite);
      }
    }
  }
}

char_size backbuffer::at(const position_t &char_on_screen) const {
  return buffer_[char_on_screen.line() * width_ + char_on_screen.index_in_line()];
}
char_size &backbuffer::at(const position_t &char_on_screen) {
  return buffer_[char_on_screen.line() * width_ + char_on_screen.index_in_line()];
  //auto &line = buffer_[char_on_screen.line()].pin();
  //return line[char_on_screen.index_in_line()];
}

void backbuffer::init(const npi_t chars_in_line, const npi_t lines_on_screen) {
    width_ = chars_in_line;
    height_ = lines_on_screen;
  buffer_.resize((chars_in_line + 1)*2 * (lines_on_screen + 1)*2 + 1 , U' ');
    for(npi_t i : std::ranges::views::iota(0, lines_on_screen * 2)){
        buffer_[i * (chars_in_line + 1)*2] = U'\n';
    }
  buffer_.back() = '\0';
  //z_buffer_.resize(chars_in_line * lines_on_screen, 0);
}

void backbuffer::send() {
  const auto pos = engine_->get_caret_index();
  bool buffer_is_changed = false;
  auto bytes = std::vector<char>(buffer_.size());
  utf8::utf32to8(buffer_.begin(), buffer_.end(),
                   std::back_inserter(bytes));
   engine_->set_selection(0, buffer_.size());
   engine_->replace_selection({bytes.begin(), bytes.end()});

   buffer_is_changed = true;
  if (buffer_is_changed) {
    engine_->set_caret_index(pos);
    scroll_.clear();
  }
}

world::world(engine_t *owner) noexcept : backbuffer(owner), engine_(owner) {
  scroll_changed_connection_ = engine_->get_on_scroll_changed().connect(
      [this](const position_t &new_scroll) { redraw_all_actors(); });

  size_changed_connection = engine_->get_on_resize().connect(
      [this](uint32_t width, uint32_t height) { redraw_all_actors(); });
}

world::~world() = default;

void world::redraw_all_actors() {
  for (const auto view = reg_.view<location_buffer>();
       const auto entity : view) {
    view.get<location_buffer>(entity).translation.mark();
  }
}
bool ecs_processors_executor::insert_processor_at(
    std::unique_ptr<ecs_processor> who,
    const std::type_info& near_with, /* typeid(ecs_processor) */
    const insert_order where) {
  if (auto it = std::ranges::find_if(
          data_,
          [&near_with](const std::unique_ptr<ecs_processor>& n) {
              return near_with == typeid(*n);
          });
      it != data_.end()) {
    switch (where) {
    case insert_order::before:
      break;
    case insert_order::after:
      ++it;
      break;
    }
    data_.insert(it, std::move(who));
    return true;
  }
  return false;
}

