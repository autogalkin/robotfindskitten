#pragma once

#include <cstdint>
#include <utility>
#include <entt/entt.hpp>
#include <Eigen/Dense>







using npi_t = int64_t; // notepad index size
class world;
class ecs_processor  // NOLINT(cppcoreguidelines-special-member-functions)
{
    
public:
    explicit ecs_processor(world* w): w_(w){}
    virtual void execute(entt::registry& reg) = 0;
    
    virtual ~ecs_processor() = default;
protected:
    [[nodiscard]] world* get_world() const {return w_;}
private:
    world* w_ = nullptr;
};


template<typename SizeType>
requires std::is_arithmetic_v<SizeType>
struct location_t : public Eigen::Vector2<SizeType>
{
    location_t(){*this << 0, 0;}
    location_t(const SizeType& line, const SizeType& index_in_line): Eigen::Vector2<SizeType>{line, index_in_line} {}

    // ReSharper disable CppNonExplicitConvertingConstructor
    
    template<typename OtherDerived>
    location_t(const Eigen::EigenBase<OtherDerived>& other): Eigen::Vector2<SizeType>{other}{}
    template<typename OtherDerived>
    location_t(const Eigen::EigenBase<OtherDerived>&& other): Eigen::Vector2<SizeType>{other}{}
    
    // ReSharper restore CppNonExplicitConvertingConstructor
    
    [[nodiscard]] SizeType& line()                      {return this->operator()(0);}
    [[nodiscard]] SizeType& index_in_line()             {return this->operator()(1);}
    [[nodiscard]] const SizeType& line() const          {return this->operator()(0);}
    [[nodiscard]] const SizeType& index_in_line() const {return this->operator()(1);}

    template<typename T>
    location_t<std::common_type_t<SizeType, T>> operator+(const location_t<T>& rhs) const {
        return  location_t<std::common_type_t<SizeType, T>>{line() + rhs.line(), index_in_line() + rhs.index_in_line()};
    }
};


using location = location_t<npi_t>;

struct velocity : location_t<int8_t>{
    explicit velocity() = default;
    [[nodiscard]] bool is_null() const {return line() == 0 && index_in_line() == 0;}
    static velocity null(){return velocity{};}
};

struct force{
    npi_t value = 1;
};
struct acceleration : force {};

namespace location_converter
{
    npi_t to_notepad_index(const location& l);
    location from_notepad_index(npi_t i);
}

struct boundbox
{
    location pivot{};
    location size {};

    boundbox operator +(const location& loc) const {return {pivot + loc, size};}
    [[nodiscard]] location center() const{
        return {pivot.line() + size.line()/2, pivot.index_in_line() + size.index_in_line()/2};
    }
    [[nodiscard]] location end() const { return {pivot + size};}
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

struct shape
{
    inline static constexpr char whitespace = ' ';
    using data_type = Eigen::Matrix<char, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;
    using initializer_from_data = Eigen::Map<const data_type, Eigen::RowMajor>;
    data_type data{};
    [[nodiscard]] boundbox bound_box() const { return {{},{ data.rows(), data.cols()}};}
};


// store a check state for ckecking changed a variable or not, automated mark as dirty when getting non const value with dirty_flag::pin()
template<typename T>
struct dirty_flag
{
    [[nodiscard]] const T& get() const noexcept {return value_.second;}
    [[nodiscard]] T& pin()  noexcept{
        mark_dirty();
        return value_.second;
    }
    dirty_flag& operator=(const T& other){
        value_.second = other;
        mark_dirty();
        return *this;
    };
    void mark_dirty() noexcept { value_.first = true;} 
    void reset_flag() noexcept {value_.first = false;}
    [[nodiscard]] bool is_changed() const noexcept {return value_.first;}
private:
    std::pair<bool, T> value_;
};

