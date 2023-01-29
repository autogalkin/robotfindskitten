#pragma once

#include "boost/signals2.hpp"



class tickable
{
public:
    
    explicit tickable() { connect_to_tick();}
    tickable(const tickable& other) : tickable() {}
    tickable(tickable&& other) noexcept : tickable() { other.tick_connection.disconnect();}
    virtual ~tickable(){ tick_connection.disconnect();}
    
    tickable& operator=(const tickable& other)
    {
        if (this == &other)
            return *this;
        connect_to_tick();
        return *this;
    }

    tickable& operator=(tickable&& other) noexcept
    {
        if (this == &other)
            return *this;
        other.tick_connection.disconnect();
        connect_to_tick();
        return *this;
    }

    
protected:
    virtual void tick() = 0;
    boost::signals2::connection tick_connection;
private:
    void connect_to_tick();
};