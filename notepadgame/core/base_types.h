#pragma once
#include <cstdint>
#include <utility>


class translation
{
public:
    translation() = default;
    translation(int64_t line, int64_t index_in_line): point_(line, index_in_line){}
    [[nodiscard]] constexpr int64_t& line()  noexcept              { return point_.first; }
    [[nodiscard]] constexpr int64_t line() const  noexcept         { return point_.first; }
    void line(int64_t const& i) noexcept                           { point_.first = i;     }
    [[nodiscard]] constexpr int64_t& index_in_line() noexcept      { return point_.second;  }
    [[nodiscard]] constexpr int64_t index_in_line() const noexcept { return point_.second;  }
    void index_in_line(int64_t const& i) noexcept                  { point_.second = i;      }
    [[nodiscard]] translation operator+(const translation& offset) const { return {line() + offset.line(), index_in_line() + offset.index_in_line()};}
    void operator+=(const translation& offset){ line() += offset.line(); index_in_line() += offset.index_in_line();}
    bool operator==(const translation& other) const noexcept  { return line() == other.line() && index_in_line() == other.index_in_line();}
    [[nodiscard]] int64_t to_index() noexcept ;
    static translation from_index(int64_t index) noexcept;
    [[nodiscard]] bool is_null() const {return line() == 0 && index_in_line() == 0;}
    
    
private:
    std::pair<int64_t, int64_t> point_{0, 0};
};

// store a check state for ckecking changed a variable or not, automated mark as dirty when getting non const value with dirty_flag::pin()
template<typename T>
struct dirty_flag
{
    [[nodiscard]] const T& get() const noexcept {return value_.second;}
    [[nodiscard]] T& pin()  noexcept
    {
        mark_dirty();
        return value_.second;
    }
    void mark_dirty() noexcept { value_.first = true;} 
    void reset_flag() noexcept {value_.first = false;}
    [[nodiscard]] bool is_changed() const noexcept {return value_.first;}
private:
    std::pair<bool, T> value_;
};