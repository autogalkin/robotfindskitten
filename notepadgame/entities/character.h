#pragma once
#include "actor.h"
#include "../core/tick.h"


class projectile;

// Tag component for the player
struct player {};

class character final : public actor
{
public:
    
    explicit character(const spawn_construct_tag _, level* l, const location& spawn,  const char& mesh): actor(_, l, spawn, mesh) {}
    virtual ~character() override{ unbind_input(); }
    
    void bind_input() const;
    void unbind_input() const {input_connection.disconnect();}
    virtual bool on_hit(const actor* const other) override;
    character(const character& other) = delete;
    character(character&& other) = delete;
    character& operator=(const character& other)  = delete;
    character& operator=(character&& other)  = delete;
private:
   boost::signals2::connection input_connection;
};
