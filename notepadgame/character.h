#pragma once
#include "input.h"
#include "core/notepad.h"


class character
{
public:
    
    inline static auto emptymesh = L' ';
    
    explicit character(const wchar_t& mesh) : mesh_(mesh){}
    character(const character& other) : character(other.mesh_) {}
    character(character&& other) noexcept : mesh_(other.mesh_) {}
    character& operator=(const character& other) {return *this = character(other);}
 
    character& operator=(character&& other) noexcept{
        std::swap(mesh_, other.mesh_);
        return *this;
    }
    virtual ~character()
    {
        unbind_input();
    }
    
    void bind_input()
    {
        // TODO key up key down manager
        auto& input = notepad::get().get_input_manager();
        connections.push_back(  input->get_signals().on_d.connect([this](const input::direction d){ if(d == input::direction::down) h_move(move::forward);  }  ));
        connections.push_back(  input->get_signals().on_a.connect([this](const input::direction d){ if(d == input::direction::down) h_move(move::backward); }  ));
        connections.push_back( input->get_signals().on_w.connect([this](const input::direction d){ if(d == input::direction::down) v_move(move::forward);  }      ));
        connections.push_back(  input->get_signals().on_s.connect([this](const input::direction d){ if(d == input::direction::down) v_move(move::backward);    }  ));
    }

    void unbind_input()
    {
        for(auto& c : connections)
        {
            c.disconnect();
        }
        connections.clear();
    }
    
    enum class move: int8_t
    {
        forward = 1,
        backward = -1
    };
    
    // move functions
    void h_move(move direction=move::forward);
    void v_move(move direction=move::forward);

    [[nodiscard]] const wchar_t& getmesh() const {return mesh_;}
    void setmesh(const wchar_t& mesh){mesh_ = mesh;}
    
private:
    wchar_t mesh_{};
    std::vector<boost::signals2::connection> connections;
};
