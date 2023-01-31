#pragma once

#include <memory>
#include <vector>
#include "actor.h"
#include "character.h"

#include "../core/base_types.h"




class world;

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

    explicit backbuffer(world* owner): world_(owner){}
    virtual ~backbuffer();
    
    void init(const int window_width);
    
    // tick operations
    void send();
    void get() const;
    
    [[nodiscard]] virtual char& at(translation char_on_screen);
    [[nodiscard]] virtual char at(translation char_on_screen) const;
    [[nodiscard]] int get_line_size() const noexcept { return line_lenght_;}
    [[nodiscard]] const translation& get_scroll() const noexcept {return scroll.get();}
    
    [[nodiscard]] translation global_position_to_buffer_position(const translation& position) const noexcept
    {
        return {position.line() - get_scroll().line(), position.index_in_line() - get_scroll().index_in_line() };
    }
    
    [[nodiscard]] bool is_in_buffer(const translation& position) const noexcept
    {
        return position.line() > get_scroll().line() || position.index_in_line() > get_scroll().index_in_line();
    }
    
private:
    static constexpr int endl = 1;
    int line_lenght_{0};
    int lines_count_{0};
    std::unique_ptr< std::vector< dirty_flag< std::vector<char> > > > buffer;
    world* world_;
    
    dirty_flag<translation> scroll;
};


class collision
{
public: 
    
    struct query_combiner
    {
        typedef std::vector<actor::collision_response> result_type;
        template <typename InputIterator>
        std::vector<actor::collision_response> operator()(InputIterator first, InputIterator last) const
        {
            
            std::vector<actor::collision_response> v;
            while (first != last)
            {
                if((*first).has_value())
                {
                    v.push_back((*first).value());
                }
                ++first;
            }
            return v;
        }
    };
    
    using signal_type = boost::signals2::signal< std::optional<actor::collision_response> (const actor::tag_type asker, const translation& position), query_combiner>;
    [[nodiscard]] signal_type& get_query() {return query;}
    
private:
    signal_type query;
    
};


class level final : public backbuffer
{
    
public:
    explicit level(world* owner) noexcept
       : backbuffer(owner)
    {
    }
    ~level() override;
    std::optional<actor*> find_actor(const actor::tag_type tag)
    {
        const auto t = actors.find(tag);
        if (t == actors.end()) return std::nullopt;
        return t->second.get();
    }

   // class spawn_error final :  public std::runtime_error{
    //public: explicit spawn_error(const std::string& message) : std::runtime_error(message.c_str()) {}
    //};
    
    // the factory method for create a new actor
    // throw spawn_error if failed
    template <std::derived_from<actor> T, typename ...Args>
    requires requires(Args&& ... args){ T(spawner{}, std::forward<Args>(args)...);}
    T* spawn_actor(const translation& spawn_location, Args&&... args) noexcept(false)
    {
        std::unique_ptr<actor> spawned =std::make_unique<T>(spawner{}, std::forward<Args>(args)...);
        spawned->set_position(spawn_location);
        spawned->set_level(this);
        auto [It, success] = actors.emplace(std::make_pair(spawned->get_id(), std::move( spawned )));

        assert(success && "the level failed to allocate a new actor while spawning, actor with the given actor::tag is already exists");
        
        It->second->connect_to_collision(&get_collision());
        if(const auto pos = global_position_to_buffer_position(spawn_location); is_in_buffer(pos))
        {
           at(pos) = It->second->getmesh();
        }
        return static_cast<T*>(It->second.get());
    }

    bool destroy_actor(const actor::tag_type tag);
    bool set_actor_location(const actor::tag_type tag, const translation new_position);
    collision& get_collision() noexcept {return collision_;}

protected:
private:
    std::unordered_map< actor::tag_type, std::unique_ptr<actor>, actor::hasher > actors{};
    collision collision_{};

};



