#pragma once
#include <cstdint>
#include <utility>

#include <boost/signals2.hpp>
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
    bool operator==(const translation& other) const noexcept  { return line() == other.line() && index_in_line() == other.index_in_line();}
    [[nodiscard]] int64_t to_index() noexcept ;
    static translation from_index(int64_t index) noexcept;
    [[nodiscard]] bool is_null() const {return line() == 0 && index_in_line() == 0;}
    
    
private:
    std::pair<int64_t, int64_t> point_{0, 0};
};

// prevent to construct actor without going through the spawn factory level::spawn_actor
struct spawner
{
private:
    spawner() = default;
    friend class level;
};
class collision;
class actor
{
public:
    friend level;
    static constexpr char whitespace = ' ';
    
    using tag_type = boost::uuids::uuid;
    using hasher = boost::hash<boost::uuids::uuid>;
    using collision_response = actor::tag_type;
    
    explicit actor(spawner key, const char mesh=whitespace) noexcept: position_(),
        tag_(boost::uuids::random_generator()()), mesh_(mesh), level_(nullptr)
    {}
    virtual ~actor()
    {
        if(collision_query_connection.connected()) {collision_query_connection.disconnect();}
    }
    
    bool operator==(actor const& rhs) const {return tag_ == rhs.tag_;}
    
    [[nodiscard]] virtual const translation& get_position() const noexcept { return position_;}
    [[nodiscard]] tag_type get_id() const {return tag_;}
    [[nodiscard]] virtual char getmesh() const noexcept {return mesh_;}
    virtual void  setmesh(const char mesh) noexcept {mesh_ = mesh;}

    actor& operator=(const actor& other) = delete;
    actor& operator=(actor&& other) noexcept = delete;
    actor(const actor& other) noexcept = delete;
    actor(actor&& other) noexcept = delete;
    actor() = delete;
    
protected:
    
    void connect_to_collision(collision* collision);
    [[nodiscard]] std::optional<actor::collision_response> collision_query_responce(const actor::tag_type asker, const translation& position) const;
    virtual bool on_hit(const actor * const other);
    
    [[nodiscard]] level* get_level() const {return level_;}
    virtual void  set_position(const translation position) noexcept {position_ = position;}
    virtual void add_position(const translation& add) noexcept {position_ += add;}
private:
    
    void set_level(level* owner){level_ = owner;}
    boost::signals2::connection collision_query_connection;
    translation position_;
    tag_type tag_ ;
    char mesh_ = whitespace;
    level* level_;
};


