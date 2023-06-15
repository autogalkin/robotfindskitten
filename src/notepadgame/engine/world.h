#pragma once
#pragma warning(push, 0)
#include <memory>
#include <vector>
#include "df/dirtyflag.h"
#pragma warning(pop)
#include "details/base_types.h"
#include "tick.h"
#include "ecs_processor_base.h"



class engine;

// the array of chars on the screen
class backbuffer 
{
public:

    explicit backbuffer(engine* owner);
    virtual ~backbuffer() = default;
    
    void init(const uint32_t w_width, const uint32_t lines_on_screen);
    
    // paste chars to the edit contol
    void send();
    
    [[nodiscard]] virtual char_size& at(const position& char_on_screen);
    [[nodiscard]] virtual char_size at(const position& char_on_screen) const;
    [[nodiscard]] position global_position_to_buffer_position(const position& position) const noexcept;
    [[nodiscard]] bool is_in_buffer(const position& global_position) const noexcept;
    void draw(const position& pivot, const shape::sprite& sh, int32_t depth);
    void erase(const position& pivot, const shape::sprite& sh, int32_t depth);
private:
    // visitor traverse all sprite matrix 
    void traverse_sprite_positions(const position& pivot, const shape::sprite& sh, const std::function<void(const position&, char_size part_of_sprite)>& visitor) const;
    // name for a \0 in buffer math operations
    static constexpr int endl = 1;
    // \n(changed to \0 if not need a \n) and \0 for a line in the buffer
    static constexpr int special_chars_count = 2;
    
    // TODO flat line buffer
    std::vector<int32_t> z_buffer{};
    using line_type = df::dirtyflag< std::vector< char_size > >;
    std::unique_ptr< std::vector< line_type > > buffer{};
    engine* engine_;
    df::dirtyflag<position> scroll_{{}, df::state::dirty};
    boost::signals2::scoped_connection scroll_changed_connection_;
    boost::signals2::scoped_connection size_changed_connection;
};



class ecs_processors_executor
{
    enum class insert_order : int8_t{
        before = 0
      , after  = 1
    };
    
public:
    explicit ecs_processors_executor(world* w): w_(w){}
    bool insert_processor_at(std::unique_ptr<ecs_processor> who
        , const std::type_info& near_with /* typeid(ecs_processor) */
        , const insert_order where=insert_order::before)
    {
        if(auto it = std::ranges::find_if(data_
                                          , [&near_with]( const std::unique_ptr<ecs_processor>& n)
                                          {
                                              return  near_with == typeid(*n);
                                          });
        it != data_.end())
        {
            switch (where)
            {
            case insert_order::before:        break;
            case insert_order::after:  ++it ; break;
            }
            data_.insert(it, std::move(who));
            return true;
        }
        return false;
    }
    void execute(entt::registry& reg, const gametime::duration delta) const
    {
        for(const auto& i : data_){
            i->execute(reg, delta);
        }
    }
    
    template<typename T, typename ...Args>
    requires std::derived_from<T, ecs_processor> && std::is_constructible_v<T, world*, Args...>
    void push(Args&&... args){
        data_.emplace_back(std::make_unique<T>(w_, std::forward<Args>(args)...));
    }
private:
   std::vector< std::unique_ptr<ecs_processor>> data_;
    world* w_;
};



class world final : public backbuffer, public tickable  // NOLINT(cppcoreguidelines-special-member-functions)
{
    
public:
    // TODO запретить копирование
    
    explicit world(engine* owner) noexcept;
    ~world() override;

    void tick(gametime::duration delta) override{
        executor_.execute(reg_, delta);
    }
    ecs_processors_executor& get_ecs_executor() {return executor_;}
    entt::registry& get_registry() {return reg_;}
    void spawn_actor(const std::function<void(entt::registry&, const entt::entity)>& for_add_components){
        const auto entity = reg_.create();
        for_add_components(reg_, entity);
    }
private:
    void redraw_all_actors();
    ecs_processors_executor executor_{this};
    entt::registry reg_;
    engine* engine_;
    boost::signals2::scoped_connection scroll_changed_connection_;
    boost::signals2::scoped_connection size_changed_connection;
    
};



