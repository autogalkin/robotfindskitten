#pragma once
#include "actor.h"
#include <vector>
#include  "boost/signals2/signal.hpp"



class character : public movable
{
public:
    
    enum class direction: int8_t
    {
        forward = 1
      , backward = -1
    };
    
    explicit character(const char& mesh)
    {
        actor::setmesh(mesh);
    }
    character(const character& other) noexcept : character(other.getmesh()) {}
    character(character&& other) noexcept  { actor::setmesh(other.getmesh()); }
    
    character& operator=(const character& other) { return *this = character(other);}
    character& operator=(character&& other) noexcept{
        setmesh(other.getmesh()); other.setmesh(' ');
        return *this;
    }

    virtual ~character() override
    {
        unbind_input();
    }
    
    void bind_input();

    void unbind_input()
    {
        for(auto& c : connections)
        {
            c.disconnect();
        }
        connections.clear();
    }
    
    // move functions
    //virtual int64_t get_position() const noexcept override;
    virtual void del_old() override;
    virtual bool spawn_new(int64_t dest) override;
    virtual void h_move(direction d = direction::forward);
    virtual void v_move(direction d=direction::forward);

    
private:
    std::vector<boost::signals2::connection> connections;
};
