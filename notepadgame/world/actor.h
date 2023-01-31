#pragma once

#include <boost/signals2.hpp>
#include <boost/container_hash/hash.hpp>

#include "boost/uuid/uuid.hpp"
#include <boost/uuid/uuid_generators.hpp>
#include "../core/base_types.h"



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
    
    explicit actor(spawner, const char mesh=whitespace) noexcept: position_(),
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


