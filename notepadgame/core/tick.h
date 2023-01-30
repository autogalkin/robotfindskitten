#pragma once

#include "boost/signals2.hpp"



class tickable
{
public:
    
    explicit tickable() { connect_to_tick();}
    tickable(const tickable& other) : tickable() {}
    tickable(tickable&& other) noexcept : tickable() { other.tick_connection.disconnect();}
    virtual ~tickable();
    tickable& operator=(const tickable& other);
    tickable& operator=(tickable&& other) noexcept;

protected:
    virtual void tick() = 0;
    boost::signals2::connection tick_connection;
private:
    void connect_to_tick();
};