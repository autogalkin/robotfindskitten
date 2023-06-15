﻿#pragma once
#pragma warning(push, 0)
#include <chrono>
#include <cstdint>
#include <utility>
#include <entt/entt.hpp>
#include <Eigen/Dense>
#include "Windows.h"
#include "df/dirtyflag.h"
#pragma warning(pop)

#include "tick.h"



using npi_t = int64_t; // notepad's index size

using char_size = char32_t; // support utf-8


class world;


template<typename SizeType>
requires std::is_arithmetic_v<SizeType>
struct vec_t : public Eigen::Vector2<SizeType>
{
    using size_type = SizeType;
    constexpr vec_t(){*this << 0, 0;}
    constexpr vec_t(const SizeType& line, const SizeType& index_in_line): Eigen::Vector2<SizeType>{line, index_in_line} {}

    template<typename OtherDerived>
    vec_t(const Eigen::EigenBase<OtherDerived>& other): Eigen::Vector2<SizeType>{other}{}
    template<typename OtherDerived>
    vec_t(const Eigen::EigenBase<OtherDerived>&& other): Eigen::Vector2<SizeType>{other}{}

    template<typename T>
    requires std::is_convertible_v<T, SizeType>
    vec_t<SizeType>& operator=(const vec_t<T>& rhs) {
        line() = rhs.line(); index_in_line() = rhs.index_in_line(); return *this;
    }
    
    [[nodiscard]] SizeType& line()                      {return this->operator()(0);}
    [[nodiscard]] SizeType& index_in_line()             {return this->operator()(1);}
    [[nodiscard]] const SizeType& line() const          {return this->operator()(0);}
    [[nodiscard]] const SizeType& index_in_line() const {return this->operator()(1);}

    
    template<typename T>
    vec_t<std::common_type_t<SizeType, T>> operator+(const vec_t<T>& rhs) const {
        return  vec_t<std::common_type_t<SizeType, T>>{line() + rhs.line(), index_in_line() + rhs.index_in_line()};
    }
    template<typename T>
    vec_t<std::common_type_t<SizeType, T>> operator*(const vec_t<T>& rhs) const {
        return  vec_t<std::common_type_t<SizeType, T>>{line() * rhs.line(), index_in_line() * rhs.index_in_line()};
    }
    template<typename T>
    requires std::is_arithmetic_v<T>
    vec_t<std::common_type_t<SizeType, T>> operator*(const T& rhs) const {
        return  vec_t<std::common_type_t<SizeType, T>>{line() * rhs, index_in_line() * rhs};
    }
    [[nodiscard]] bool is_null() const {return line() == static_cast<SizeType>(0) && index_in_line() == static_cast<SizeType>(0);}
    static vec_t<SizeType> null(){return vec_t<SizeType>{static_cast<SizeType>(0), static_cast<SizeType>(0)};}
};

// notepad's col-row position
using position = vec_t<npi_t>;

// an actor location, used for smooth move
using location = vec_t<double>;


struct uniform_movement_tag {};
struct non_uniform_movement_tag {};


enum class direction : int8_t{
    forward =  1,
    reverse = -1
};

namespace direction_converter{
    template<typename T>
    requires std::is_enum_v<T>
    static T invert(const T d){return static_cast<T>(static_cast<int>(d) * -1);}
}



namespace timeline
{
    // calls a given function every tick to the end lifetime
    struct what_do{
        std::function<void(entt::registry& , entt::entity, direction)> work;
    };
    struct eval_direction{
        direction value;
    };

}

struct visible_in_game{};


namespace position_converter
{
    npi_t to_notepad_index(const position& l);
    position from_notepad_index(npi_t i);
    inline position from_location(const location& location) {return {std::lround(location.line()), std::lround(location.index_in_line())}; }
}

struct velocity : vec_t<float>{
    velocity() = default;
    velocity(const vec_t<float>& l) : vec_t<float>(l){}
    template<typename T>
    velocity(const Eigen::EigenBase<T>& other): vec_t<float>{other}{}
    template<typename T>
    velocity(const Eigen::EigenBase<T>&& other): vec_t<float>{other}{}
    velocity(const float line, const float index_in_line) : vec_t<float>{line, index_in_line}{}
    
};

namespace life
{
    struct begin_die{};
    
    struct lifetime{
        gametime::duration duration ;
    };

    struct death_last_will{
        std::function<void(entt::registry&, entt::entity)> wish;
    }; 
}




struct boundbox
{
    position pivot{};
    position size {};

    boundbox operator +(const position& loc) const {return {pivot + loc, size};}
    [[nodiscard]] position center() const{
        return {pivot.line() + size.line()/2, pivot.index_in_line() + size.index_in_line()/2};
    }
    [[nodiscard]] position end() const { return {pivot + size};}
    [[nodiscard]] bool contains(const boundbox& b) const
    {
        return
            pivot.line() <= b.pivot.line()
        &&  end().line() <= b.end().line()
        &&  pivot.index_in_line() <= b.pivot.index_in_line()
        &&  end().index_in_line() <= b.end().index_in_line();
    }
    [[nodiscard]] bool intersects(const boundbox& b) const
    {
        return !(
            pivot.line() >= b.end().line()
        ||  end().line() <= b.pivot.line()
        ||  pivot.index_in_line() >= b.end().index_in_line()
        ||  end().index_in_line() <= b.pivot.index_in_line());
    }
};




namespace shape
{
    inline static constexpr char whitespace = ' ';
    
    struct render_direction{
        direction value = direction::forward;
    };
   
    struct sprite{
        
        using data_type = Eigen::Matrix< char_size, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;
        using from_data = Eigen::Map<const data_type, Eigen::RowMajor>;
        data_type data{};
        [[nodiscard]] boundbox bound_box() const
        {
            
            return {{},{ data.rows(), data.cols()}};
        }
    };
  
    struct sprite_animation{
        [[nodiscard]] sprite& current_sprite() {return sprts[rendering_i];}
        [[nodiscard]] const sprite& current_sprite() const {return sprts[rendering_i];}
        std::vector<sprite> sprts;
        uint8_t rendering_i = 0;
    };
    
    struct on_change_direction{
        std::function<void(direction, sprite_animation&)> call;
    };
}

struct color_range
{
    COLORREF start{RGB(0, 0, 0)};
    COLORREF end{RGB(255, 255, 255)};
};

struct location_buffer
{
    location current{};
    df::dirtyflag<location> translation{{}, df::state::dirty};
};

struct z_depth {
    int32_t value;
};