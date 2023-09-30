#pragma once
#include <chrono>
#include <cstdint>
#include <entt/entt.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/fwd.hpp>
#include <type_traits>
#include <utility>
#include "engine/time.h"
#include "Windows.h"

#include <boost/geometry.hpp>
#include <glm/vec2.hpp>

using npi_t = int32_t; // notepad's index size

using char_size = char;

template <typename T> using vec = glm::vec<2, T>;

// an actor location, used for smooth move
using loc = vec<double>;
// notepad's col-row position
using pos = vec<npi_t>;

namespace pos_declaration {
/*
   s         X(index in line)
   +------+-->
   |      |
   |      |
   |      |
   +------+
   |      e
   v
   Y (line)
*/
static constexpr size_t S = 0;
static constexpr size_t E = 1;
static constexpr size_t X = 0;
static constexpr size_t Y = 1;
} // namespace pos_declaration

struct sprite {
    inline static constexpr char whitespace = ' ';
    size_t width;
    std::basic_string<char_size> data;
    sprite(std::basic_string<char_size> str) : width(str.size()), data(str) {}
    pos bounds() const noexcept { return {width, data.size() / width}; }
};
static sprite make_sprite(std::string s) {
    // trim right and left
    auto start = std::find_if(
        s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); });
    auto end = std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
                   return !std::isspace(ch);
               }).base();

    auto for_each_line = [](auto start, auto end, auto pred) {
        for (auto it = std::find(start, end, '\n'); it != end;
             it = std::find(++it, end, '\n')) {
            pred(it);
        }
        // last line to end
        pred(end);
    };
    size_t max_len = 0;
    size_t lines = 1;
    for_each_line(start, end, [start, &lines, &max_len](auto it) mutable {
        ++lines;
        size_t new_len = std::distance(start, it);
        if (new_len > max_len) {
            max_len = new_len;
        }
        start = ++it;
    });
    std::string out{};
    out.reserve(max_len * lines);
    for_each_line(start, end, [start, &out, max_len](auto it) mutable {
        std::fill_n(std::copy(start, it, std::back_inserter(out)),
                    max_len - std::distance(start, it), ' ');
        start = ++it;
    });
    auto sp = sprite{out};
    sp.width = max_len;
    return sp;
};

struct sprite_view {
    size_t width;
    std::string_view data;
    sprite_view(const sprite& s)
        : data(s.data.begin(), s.data.end()), width(s.width) {}
};

template <typename T> class dirty_flag {
    bool changed_ = true;
    T v_;

  public:
    explicit constexpr dirty_flag() = default;
    constexpr dirty_flag(T&& v) noexcept : v_(std::forward<T>(v)) {}
    template <typename... Args>
        requires std::is_constructible_v<T, Args...>
    explicit constexpr dirty_flag(Args&&... args) noexcept
        : v_(std::forward<Args>(args)...) {}

    [[nodiscard]] T& pin() noexcept {
        changed_ = true;
        return v_;
    }
    operator T() const { return v_; }
    operator bool() const noexcept { return changed_; }
    [[nodiscard]] const T& get() const noexcept { return v_; }
    [[nodiscard]] bool is_changed() const noexcept { return changed_; }
    void clear() noexcept { changed_ = false; }
    void mark() noexcept { changed_ = true; }
};
