
#include "df/dirtyflag.h"
#pragma warning(push, 0)
#include "range/v3/view/enumerate.hpp"
#include "range/v3/view/filter.hpp"
#include "utf8cpp/utf8.h"
#pragma warning(pop)
#include "engine.h"
#include "notepader.h"
#include "world.h"

backbuffer::backbuffer(engine *owner) : engine_(owner) {
  scroll_changed_connection_ = engine_->get_on_scroll_changed().connect(
      [this](const position &new_scroll) {
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

position backbuffer::global_position_to_buffer_position(
    const position &position) const noexcept {
  return {position.line() - scroll_.get().line(),
          position.index_in_line() - scroll_.get().index_in_line()};
}

bool backbuffer::is_in_buffer(const position &global_position) const noexcept {
  return global_position.line() >= scroll_.get().line() &&
         global_position.index_in_line() >= scroll_.get().index_in_line() &&
         global_position.index_in_line() <=
             static_cast<int>(engine_->get_window_width()) /
                     engine_->get_char_width() +
                 scroll_.get().index_in_line() &&
         global_position.line() <=
             engine_->get_lines_on_screen() + scroll_.get().line();
}

void backbuffer::draw(const position &pivot, const shape::sprite &sh,
                      int32_t depth) {

  traverse_sprite_positions(
      pivot, sh, [this, depth](const position &p, const char_size ch) {
        int32_t z_index =
            p.line() * static_cast<int>(engine_->get_window_width()) +
            p.index_in_line();
        if (depth >= this->z_buffer[z_index]) {
          this->z_buffer[z_index] = depth;
          at(p) = ch;
        }
      });
}

void backbuffer::erase(const position &pivot, const shape::sprite &sh,
                       int32_t depth) {
  traverse_sprite_positions(
      pivot, sh, [this, depth](const position &p, char_size) {
        int32_t z_index =
            p.line() * static_cast<int>(engine_->get_window_width()) +
            p.index_in_line();
        if (this->z_buffer[z_index] <= depth) {
          at(p) = shape::whitespace;
        }
      });
}

void backbuffer::traverse_sprite_positions(
    const position &pivot, const shape::sprite &sh,
    const std::function<void(const position &, char_size)> &visitor) const {
  const position screen_pivot = global_position_to_buffer_position(pivot);

  for (auto rows = sh.data.rowwise();
       auto [line, row] : rows | ranges::views::enumerate) {
    for (int byte_i{-1};
         const auto part_of_sprite :
         row | ranges::views::filter([&byte_i](const char_size c) {
           ++byte_i;
           return c != shape::whitespace;
         })) {
      if (position p =
              screen_pivot + position{static_cast<npi_t>(line), byte_i};
          p.index_in_line() >= 0 &&
          p.index_in_line() <= static_cast<int>(engine_->get_window_width()) /
                                   engine_->get_char_width() &&
          p.line() >= 0 && p.line() <= engine_->get_lines_on_screen()) {
        visitor(p, part_of_sprite);
      }
    }
  }
}

char_size backbuffer::at(const position &char_on_screen) const {
  auto &line = (*buffer)[char_on_screen.line()].get();
  return line[char_on_screen.index_in_line()];
}
char_size &backbuffer::at(const position &char_on_screen) {
  auto &line = (*buffer)[char_on_screen.line()].pin();
  return line[char_on_screen.index_in_line()];
}

void backbuffer::init(const uint32_t w_width, const uint32_t lines_on_screen) {
  const uint32_t line_lenght = w_width / engine_->get_char_width();

  if (!buffer)
    buffer = std::make_unique<std::vector<line_type>>(
        lines_on_screen, line_type{{}, df::state::dirty});
  else
    buffer->resize(lines_on_screen + 1, line_type{{}, df::state::dirty});
  for (auto &line : *buffer) {
    line.pin().resize(line_lenght + 1 + special_chars_count, U' ');
    std::ranges::fill(line.pin(), U' ');

    *(line.pin().end() - 1 /*past-the-end*/ - endl) = U'\n';
    line.pin().back() = U'\0';
  }
  z_buffer.resize(w_width * lines_on_screen, 0);
}

void backbuffer::send() {
  const auto pos = engine_->get_caret_index();
  bool buffer_is_changed = false;

  for (int enumerate{-1};
       auto &line : *buffer // | ranges::views::enumerate
                        | ranges::views::filter([this, &enumerate](auto &l) {
                            ++enumerate;
                            return l.is_dirty() || scroll_.is_dirty();
                          })) {

    const npi_t lnum = scroll_.get().line() + enumerate;
    const npi_t fi = engine_->get_first_char_index_in_line(lnum);

    npi_t endsel = engine_->get_line_lenght(
        lnum); // std::max(0, line.get().pasted_bytes_count-1);
    char_size ch_end{'\0'};
    if (lnum >= engine_->get_lines_count() - 1) {
      ch_end = '\n';
    } else {
      endsel -= endl;
    }

    *(line.pin().end() - 1 /*past-the-end*/ - endl) =
        ch_end; // enable or disable eol

    std::vector<char> bytes{};
    utf8::utf32to8(line.get().begin(), line.get().end(),
                   std::back_inserter(bytes));

    engine_->set_selection(fi + scroll_.get().index_in_line(), fi + endsel);
    engine_->replace_selection({bytes.begin(), bytes.end()});

    line.clear();
    buffer_is_changed = true;
  }

  if (buffer_is_changed) {
    engine_->set_caret_index(pos);
    scroll_.clear();
  }
}

world::world(engine *owner) noexcept : backbuffer(owner), engine_(owner) {
  scroll_changed_connection_ = engine_->get_on_scroll_changed().connect(
      [this](const position &new_scroll) { redraw_all_actors(); });

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
