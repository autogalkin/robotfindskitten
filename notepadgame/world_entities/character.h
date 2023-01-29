#pragma once
#include "actor.h"
#include <vector>
#include  "boost/signals2/signal.hpp"
#include "../core/tick.h"


class character final : public mesh_actor , public i_movable
{
public:
    explicit character(const char& mesh): mesh_actor(mesh) {}
    character(const character& other) noexcept : mesh_actor(other){}
    character(character&& other) noexcept : mesh_actor(std::move(other)), input_connection(std::move(other.input_connection)){}
    character& operator=(const character& other);
    character& operator=(character&& other) noexcept;
    
    virtual ~character() override{ unbind_input(); }
    // TODO bind input and constructors
    void bind_input();
    void unbind_input() const {input_connection.disconnect();}
    void move(const translation& offset) override;

private:
   boost::signals2::connection input_connection;
};
