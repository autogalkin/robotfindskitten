#pragma once
#include "actor.h"

#include "../core/tick.h"


class projectile final : public actor, public tickable
{
public:
    
    explicit projectile(const spawner key, translation direction={0, 1}, const int lifetime = 5, const char mesh='-') : actor(key, mesh), tickable(), direction_(std::move(direction)), lifetime_(lifetime){}
    projectile& operator=(const projectile& other) = delete;
    projectile& operator=(projectile&& other) = delete;
    projectile(const projectile& other) = delete;
    projectile(projectile&& other) = delete;
    ~projectile() override = default;

protected:
    void tick() override;

private:
    translation direction_;
    int lifetime_ = 5;
};
