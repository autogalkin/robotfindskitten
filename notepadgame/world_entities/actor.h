#pragma once
#include <cstdint>
#include <string>
#include "../core/notepader.h"

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
    
    virtual void del_old()
    {
        auto p = get_position();
        auto& w = notepader::get().get_world();
        w->set_selection(p-1, p);
        w->replace_selection(std::string{entities::whitespace});
    }
    virtual bool spawn_new(const int64_t dest)
    {
        auto& w = notepader::get().get_world();
        w->set_selection(dest-1, dest);
        w->replace_selection(std::string{getmesh()});
        return true;
    }

};
