#pragma once
#include "actor.h"
#include <vector>



#include  "boost/signals2/signal.hpp"
#include "../core/tick.h"


class projectile;

class character final : public actor
{
public:
    
    explicit character(const spawner key, const char& mesh): actor(key, mesh) {}
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
