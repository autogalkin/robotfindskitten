#pragma once
#include "df/dirtyflag.h"
#include <Eigen/Dense>
#include <chrono>
#include <cstdint>
#include <entt/entt.hpp>
#include <type_traits>
#include <utility>
#include "engine/time.h"
#include "Windows.h"

using npi_t = int64_t; // notepad's index size

using char_size = char;

class world;

template <typename SizeType>
    requires std::is_arithmetic_v<SizeType>
struct vec_t : public Eigen::Vector2<SizeType> {
    using size_type = SizeType;
    /* implicit*/ vec_t() { *this << 0, 0; }
    vec_t(const SizeType& line, const SizeType& index_in_line) noexcept
        : Eigen::Vector2<SizeType>{line, index_in_line} {}
    // implicit from eigens
    template <typename OtherDerived>
    vec_t(const Eigen::EigenBase<OtherDerived>& other) noexcept
        : Eigen::Vector2<SizeType>{other} {}
    template <typename OtherDerived>
    vec_t(const Eigen::EigenBase<OtherDerived>&& other) noexcept
        : Eigen::Vector2<SizeType>{other} {}

    template <typename T>
        requires std::is_convertible_v<T, SizeType>
    vec_t<SizeType>& operator=(const vec_t<T>& rhs) noexcept {
        line() = rhs.line();
        index_in_line() = rhs.index_in_line();
        return *this;
    }

    [[nodiscard]] SizeType& line() noexcept { return this->operator()(0); }
    [[nodiscard]] SizeType& index_in_line() noexcept {
        return this->operator()(1);
    }
    [[nodiscard]] const SizeType& line() const noexcept {
        return this->operator()(0);
    }
    [[nodiscard]] const SizeType& index_in_line() const noexcept {
        return this->operator()(1);
    }

    template <typename T>
    vec_t<std::common_type_t<SizeType, T>>
    operator+(const vec_t<T>& rhs) const noexcept {
        return vec_t<std::common_type_t<SizeType, T>>{
            line() + rhs.line(), index_in_line() + rhs.index_in_line()};
    }
    template <typename T>
    vec_t<std::common_type_t<SizeType, T>>
    operator*(const vec_t<T>& rhs) const noexcept {
        return vec_t<std::common_type_t<SizeType, T>>{
            line() * rhs.line(), index_in_line() * rhs.index_in_line()};
    }
    template <typename T>
        requires std::is_arithmetic_v<T>
    vec_t<std::common_type_t<SizeType, T>>
    operator*(const T& rhs) const noexcept {
        return vec_t<std::common_type_t<SizeType, T>>{line() * rhs,
                                                      index_in_line() * rhs};
    }
    [[nodiscard]] bool is_null() const noexcept {
        return line() == static_cast<SizeType>(0) &&
               index_in_line() == static_cast<SizeType>(0);
    }
    static vec_t<SizeType> make_null() noexcept {
        return vec_t<SizeType>{static_cast<SizeType>(0),
                               static_cast<SizeType>(0)};
    }
};

// TODO not trivially copyable
// notepad's col-row position
using position_t = vec_t<npi_t>;

// an actor location, used for smooth move
using location = vec_t<double>;

enum class direction : int8_t { forward = 1, reverse = -1 };

namespace direction_converter {
template <typename T>
    requires std::is_enum_v<T>
static T invert(const T d) {
    return static_cast<T>(static_cast<int>(d) * -1);
}
} // namespace direction_converter



struct visible_in_game {};

namespace position_converter {
inline position_t from_location(const location& location) {
    return {std::lround(location.line()),
            std::lround(location.index_in_line())};
}
} // namespace position_converter



namespace shape {
inline static constexpr char whitespace = ' ';

struct render_direction {
    direction value = direction::forward;
};

struct sprite {

    using data_type = Eigen::Matrix<char_size, Eigen::Dynamic, Eigen::Dynamic,
                                    Eigen::RowMajor>;
    using from_data = Eigen::Map<const data_type, Eigen::RowMajor>;
    data_type data{};
    [[nodiscard]] position_t bounds() const {

        return {data.rows(), data.cols()};
    }
};

struct sprite_animation {
    [[nodiscard]] sprite& current_sprite() { return sprts[rendering_i]; }
    [[nodiscard]] const sprite& current_sprite() const {
        return sprts[rendering_i];
    }
    std::vector<sprite> sprts;
    uint8_t rendering_i = 0;
};


} // namespace shape



struct location_buffer {
    location current{};
    df::dirtyflag<location> translation{{}, df::state::dirty};
};


