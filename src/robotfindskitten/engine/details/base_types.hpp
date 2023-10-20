/**
 * @file
 * @brief The Project type system.
 *
 * This header describes building blocks for the project.
 * Types mostly based on glm primitives.
 */

#ifndef _CPP_PROJECTS_ROBOTFINDSKITTEN_SRC_ROBOTFINDSKITTEN_ENGINE_DETAILS_BASE_TYPES_HPP
#define _CPP_PROJECTS_ROBOTFINDSKITTEN_SRC_ROBOTFINDSKITTEN_ENGINE_DETAILS_BASE_TYPES_HPP

#include <cstdint>
#include <iterator>
#include <string>
#include <string_view>
#include <type_traits>
#include <random>

#include <glm/vec2.hpp>

using npi_t = int32_t; // notepad's index size

using char_size = char;
using random_t = std::mt19937;

template<typename T>
using vec = glm::vec<2, T>;

// an actor location, used for smooth move
using loc = vec<double>;

// notepad's col-row position
using pos = vec<npi_t>;

class sprite {
    uint16_t width_{};
    // expect CharT null terminator;
    std::basic_string<char_size> data_;
    [[nodiscard]] static sprite normilize_from_string(std::string s);
    sprite(uint16_t width, std::basic_string<char_size> str)
        : width_(width), data_(std::move(str)) {}

public:
    inline static constexpr char whitespace = ' ';
    struct should_normalize_tag {};
    struct unchecked_construct_tag {};
    explicit sprite(unchecked_construct_tag /*tag*/,
                    std::basic_string<char_size> str)
        : width_(static_cast<uint16_t>(str.size())), data_(std::move(str)) {}
    explicit sprite(should_normalize_tag /*tag*/,
                    std::basic_string<char_size> str) {
        *this = normilize_from_string(std::move(str));
    }
    [[nodiscard]] pos bounds() const noexcept {
        return {width_, data_.size() / width_};
    }
    [[nodiscard]] uint16_t width() const noexcept {
        return width_;
    }
    [[nodiscard]] std::string_view data() const noexcept {
        return data_;
    }
};

class sprite_view {
    uint16_t width_;
    std::string_view data_;

public:
    // NOLINTNEXTLINE(google-explicit-constructor)
    sprite_view(const sprite& s): width_(s.width()), data_(s.data()) {}

    [[nodiscard]] uint16_t width() const noexcept {
        return width_;
    }
    [[nodiscard]] std::string_view data() const noexcept {
        return data_;
    }
};
inline sprite sprite::normilize_from_string(std::string s) {
    // trim right and left
    auto start = std::find_if(
        s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); });
    auto end = std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
                   return !std::isspace(ch);
               }).base();

    auto for_each_line = [](auto start, auto end, auto pred) {
        for(auto it = std::find(start, end, '\n'); it != end;
            it = std::find(++it, end, '\n')) {
            pred(it);
        }
        // last line to end
        pred(end);
    };
    uint16_t max_len = 0;
    size_t lines = 1;
    for_each_line(start, end, [start, &lines, &max_len](auto it) mutable {
        ++lines;
        size_t new_len = std::distance(start, it);
        if(new_len > max_len) {
            max_len = static_cast<uint16_t>(new_len);
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
    return {max_len, out};
};

template<typename T>
class dirty_flag {
    bool changed_ = true;
    T v_;

public:
    explicit constexpr dirty_flag() = default;
    // NOLINTNEXTLINE(google-explicit-constructor)
    constexpr dirty_flag(T&& v) noexcept: v_(std::forward<T>(v)) {}
    template<typename... Args>
        requires std::is_constructible_v<T, Args...>
    explicit constexpr dirty_flag(Args&&... args) noexcept
        : v_(std::forward<Args>(args)...) {}

    [[nodiscard]] T& pin() noexcept {
        changed_ = true;
        return v_;
    }
    // NOLINTNEXTLINE(google-explicit-constructor)
    operator T&() const noexcept
        requires(!std::is_convertible_v<T, bool>)
    {
        return v_;
    }
    // NOLINTNEXTLINE(google-explicit-constructor)
    operator bool() const noexcept {
        return changed_;
    }
    [[nodiscard]] const T& get() const noexcept {
        return v_;
    }
    [[nodiscard]] bool is_changed() const noexcept {
        return changed_;
    }
    void clear() noexcept {
        changed_ = false;
    }
    void mark() noexcept {
        changed_ = true;
    }
};

#endif
