#pragma once

#include <memory>
#include <vector>


#include "../core/base_types.h"
#include "../entities/actor.h"






class engine;

// the array of chars on the screen
class backbuffer 
{
    friend class world;
    friend class notepader; // for update the scroll
public:
    backbuffer(const backbuffer& other) = delete;
    backbuffer(backbuffer&& other) noexcept;
    backbuffer& operator=(const backbuffer& other) = delete;
    backbuffer& operator=(backbuffer&& other) noexcept;

    explicit backbuffer(engine* owner): world_(owner){}
    virtual ~backbuffer();
    
    void init(const int window_width);
    
    // tick operations
    void send();
    void get() const;
    
    [[nodiscard]] virtual char& at(const location& char_on_screen);
    [[nodiscard]] virtual char at(const location& char_on_screen) const;
    [[nodiscard]] int get_line_size() const noexcept { return line_lenght_;}
    [[nodiscard]] const location& get_scroll() const noexcept {return scroll.get();}
    
    [[nodiscard]] location global_position_to_buffer_position(const location& position) const noexcept
    {
        return {position.line - get_scroll().line, position.index_in_line - get_scroll().index_in_line };
    }
    
    [[nodiscard]] bool is_in_buffer(const location& position) const noexcept
    {
        return position.line > get_scroll().line || position.index_in_line > get_scroll().index_in_line;
    }
    
private:
    static constexpr int endl = 1;
    int line_lenght_{0};
    int lines_count_{0};
    using line_type = dirty_flag< std::vector<char> >;
    std::unique_ptr< std::vector< line_type > > buffer;
    engine* world_;
    
    dirty_flag<location> scroll;
};




class world final : public backbuffer
{
    
public:
    explicit world(engine* owner) noexcept
       : backbuffer(owner)
    {
    }
    ~world() override;
    std::optional<actor*> find_actor(const actor::tag_type tag)
    {
        const auto t = actors.find(tag);
        if (t == actors.end()) return std::nullopt;
        return t->second.get();
    }
    
    // the factory method for create a new actor
    // throw spawn_error if failed
    template <std::derived_from<actor> T, typename ...Args>
    requires requires(world* l, Args&& ... args){ T(spawn_construct_tag{}, l, std::forward<Args>(args)...);}
    T* spawn_actor(Args&&... args) noexcept
    {
        std::unique_ptr<actor> spawned =std::make_unique<T>(spawn_construct_tag{}, this, std::forward<Args>(args)...);
        auto [It, success] = actors.emplace(std::make_pair(spawned->get_id(), std::move( spawned )));

        assert(success && "the level failed to allocate a new actor while spawning, actor with the given actor::tag is already exists");
        
        if(const auto pos = global_position_to_buffer_position(It->second->get_pivot()); is_in_buffer(pos))
        {
            //TODO draw shape
           at(pos) = It->second->getmesh();
        }
        return static_cast<T*>(It->second.get());
    }
    
    
    bool destroy_actor(const actor::tag_type tag);
    bool set_actor_location(const actor::tag_type tag, const location new_position);
    

protected:
private:
    std::unordered_map< actor::tag_type, std::unique_ptr<actor>, actor::hasher > actors{};

    entt::registry reg;
};



