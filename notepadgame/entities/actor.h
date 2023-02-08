#pragma once

#include <array>

#include <boost/signals2.hpp>
#include <boost/container_hash/hash.hpp>

#include "boost/uuid/uuid.hpp"
#include <boost/uuid/uuid_generators.hpp>
#include "../core/base_types.h"
#include <Eigen/Dense>


struct shape
{
    static constexpr char whitespace = ' ';
    using type                  = Eigen::Matrix<char, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;
    using initializer_from_data = Eigen::Map<const shape::type,   Eigen::RowMajor>;
    type data{};
};


// prevent to construct actor without going through the spawn factory level::spawn_actor
struct spawn_construct_tag
{
private:
    spawn_construct_tag() = default;
    friend class world;
};


class actor
{
public:
    friend world;
    
    using tag_type = boost::uuids::uuid;
    using hasher = boost::hash<boost::uuids::uuid>;
    using collision_response = actor::tag_type;
    
    explicit actor(spawn_construct_tag, world* w
        , location spawn_location, const char mesh=shape::whitespace) noexcept:
    pivot_(std::move(spawn_location)), 
    tag_(boost::uuids::random_generator()()), mesh_(mesh), world_(w)
    {}
    virtual ~actor() = default;
    bool operator==(actor const& rhs) const {return tag_ == rhs.tag_;}
    
    [[nodiscard]] virtual const location& get_pivot() const noexcept { return pivot_;}
    [[nodiscard]] tag_type get_id() const {return tag_;}
    [[nodiscard]] virtual char getmesh() const noexcept {return mesh_;}
    virtual void  setmesh(const char mesh) noexcept {mesh_ = mesh;}

    [[nodiscard]] const shape& getmesh_(){return mesh1_;}
    
    actor& operator=(const actor& other)  = delete;
    actor& operator=(actor&& other)       = delete;
    actor(const actor& other)             = delete;
    actor(actor&& other)                  = delete;

    
protected:
    
    //void connect_to_collision(collision* collision);
    [[nodiscard]] std::optional<actor::collision_response> collision_query_responce(const actor::tag_type asker, const location& position) const;
    virtual bool on_hit(const actor * const other);
    
    [[nodiscard]] world* get_world() const {return world_;}
    virtual void  set_position(const location& position) noexcept {pivot_ = position;}
    virtual void add_position(const location& add) noexcept
    {
        pivot_ += add;
    }
private:
    
    boost::signals2::scoped_connection collision_query_connection;
    
    location pivot_;
    tag_type tag_ ;
    char mesh_ = shape::whitespace;
    
    shape mesh1_{};
    world* world_;
};

