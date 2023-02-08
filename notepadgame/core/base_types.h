#pragma once
#include "Windows.h"
#include <cstdint>
#include <iterator>
#include <ranges>
#include <utility>
#include <entt/entt.hpp>
#include "../world/actor.h"

#include "Eigen/Dense"


using npi_t = int64_t; // notepad index size

class ecs_processor  // NOLINT(cppcoreguidelines-special-member-functions)
{
public:
    virtual ~ecs_processor() = default;
    virtual void execute(entt::registry& reg) = 0;
};


template<typename SizeType>
struct location_t : public Eigen::Vector2<SizeType>
{
    location_t(){*this << 0, 0;}
    location_t(const SizeType& line, const SizeType& index_in_line): Eigen::Vector2<SizeType>{line, index_in_line} {}
    
    template<typename OtherDerived>
    // ReSharper disable CppNonExplicitConvertingConstructor
    location_t(const Eigen::EigenBase<OtherDerived>& other): Eigen::Vector2<SizeType>{other}{}
    template<typename OtherDerived>
    location_t(const Eigen::EigenBase<OtherDerived>&& other): Eigen::Vector2<SizeType>{other}{}
    // ReSharper restore CppNonExplicitConvertingConstructor
    [[nodiscard]] SizeType& line()                      {return this->operator()(0);}
    [[nodiscard]] SizeType& index_in_line()             {return this->operator()(1);}
    [[nodiscard]] const SizeType& line() const          {return line();}
    [[nodiscard]] const SizeType& index_in_line() const {return index_in_line();}
    
};

using location = location_t<npi_t>;

struct velocity : location{};
struct force{
    npi_t value = 1;
};
struct movement_force : force {};


namespace location_converter
{
    npi_t to_notepad_index(const location& l);
    location from_notepad_index(npi_t i);
}


// store a check state for ckecking changed a variable or not, automated mark as dirty when getting non const value with dirty_flag::pin()
template<typename T>
struct dirty_flag
{
    [[nodiscard]] const T& get() const noexcept {return value_.second;}
    [[nodiscard]] T& pin()  noexcept{
        mark_dirty();
        return value_.second;
    }
    void mark_dirty() noexcept { value_.first = true;} 
    void reset_flag() noexcept {value_.first = false;}
    [[nodiscard]] bool is_changed() const noexcept {return value_.first;}
private:
    std::pair<bool, T> value_;
};

struct color_range
{
    COLORREF start{RGB(0, 0, 0)};
    COLORREF end{RGB(255, 255, 255)};
};