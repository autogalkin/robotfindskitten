#pragma once
#include <cstdint>
#include <utility>

#include <boost/container_hash/hash.hpp>

#include "boost/uuid/uuid.hpp"
#include <boost/uuid/uuid_generators.hpp>


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
    [[nodiscard]] int64_t to_index() noexcept ;
    static translation from_index(int64_t index) noexcept;
    bool is_null() const {return line() == 0 && index_in_line() == 0;}
    
private:
    std::pair<int64_t, int64_t> point_{0, 0};
};


class actor
{
    friend class level;
public:
    using tag_type = boost::uuids::uuid;
    using hasher = boost::hash<boost::uuids::uuid>;

    explicit actor(translation position = {}) noexcept
        : position_(std::move(position)),
          tag_(boost::uuids::random_generator()())
    {}
    actor(const actor& other) noexcept = default;
    actor(actor&& other) noexcept
        : position_(std::move(other.position_)),
          tag_(other.tag_)
    {}
    
    actor& operator=(const actor& other) noexcept
    {
        if (this == &other)
            return *this;
        position_ = other.position_;
        return *this;
    }

    actor& operator=(actor&& other) noexcept
    {
        if (this == &other)
            return *this;
        position_ = std::move(other.position_);
        tag_ = other.tag_;
        return *this;
    }
    
    virtual ~actor() = default;
    bool operator==(actor const& rhs) const {return tag_ == rhs.tag_;}
    
    [[nodiscard]] virtual const translation& get_position() const noexcept { return position_;}
    [[nodiscard]] tag_type get_id() const {return tag_;}
protected:
    virtual void  set_position(const translation position) noexcept {position_ = position;}
    virtual void add_position(const translation& add) noexcept {position_ += add;}
private:
    translation position_;
    tag_type tag_ ;
};


class i_movable    // NOLINT(cppcoreguidelines-special-member-functions)
{
public:
    virtual ~i_movable() = default;
    virtual void move(const translation& offset) = 0;
};


class mesh_actor : public actor
{
public:
    static constexpr char whitespace = ' ';
    explicit mesh_actor(const char mesh=whitespace) : actor(), mesh_(mesh){}
    mesh_actor(const mesh_actor& other) noexcept = default;
    mesh_actor(mesh_actor&& other) noexcept : actor(std::move(other)), mesh_(other.mesh_){}
    mesh_actor& operator=(const mesh_actor& other);
    mesh_actor& operator=(mesh_actor&& other) noexcept;
    ~mesh_actor() override = default;
    
    [[nodiscard]] virtual char getmesh() const noexcept {return mesh_;}
    virtual void  setmesh(const char mesh) noexcept {mesh_ = mesh;}

private:
    char mesh_ = whitespace;
};

