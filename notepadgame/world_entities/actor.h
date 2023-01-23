#pragma once
#include <cstdint>



namespace entities
{
    static constexpr char whitespace = ' ';
    
}


class actor
{
public:
    virtual ~actor() = default;
    [[nodiscard]] virtual char getmesh() const noexcept {return mesh_;}
    [[nodiscard]] virtual int64_t get_position() const noexcept {return position_;}
    virtual void  setmesh(const char mesh) noexcept {mesh_ = mesh;}
    
protected:
    virtual void  set_position(const int64_t index) noexcept {position_ = index;}
    
private:
    int64_t  position_ = 0;
    char mesh_ = 0;
};


class movable: public actor
{
public:
    
    virtual void del_old();
    virtual bool spawn_new(const int64_t dest);
};
