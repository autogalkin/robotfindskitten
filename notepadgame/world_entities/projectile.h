#pragma once
#include "actor.h"

#include "../core/tick.h"


class projectile final : public mesh_actor, public i_movable, public tickable
{
public:
    projectile(const projectile& other) : projectile( other.direction_, other.lifetime_, other.getmesh()){}
    projectile(projectile&& other) noexcept :  mesh_actor(other.getmesh()), direction_(std::move(other.direction_)), lifetime_(other.lifetime_) {}
    explicit projectile( translation direction={0, 1}, const int lifetime = 5, const char mesh='-') : mesh_actor(mesh), tickable(), direction_(std::move(direction)), lifetime_(lifetime){}
    projectile& operator=(const projectile& other);
    projectile& operator=(projectile&& other) noexcept;
    ~projectile() override {}
    
    void move(const translation& offset) override;

protected:
    void tick() override;

private:
    translation direction_;
    int lifetime_ = 5;
};
