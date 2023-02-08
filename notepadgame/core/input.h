#pragma once
#include <queue>
#include <entt/entt.hpp>
#include "boost/signals2.hpp"
#include <Windows.h>

#include "base_types.h"
#include "tick.h"




class input final : public tickable, public ecs_processor
{
public:
    
    // https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
    enum class key : WPARAM {
          w = 0x57
        , a = 0x41
        , s = 0x53
        , d = 0x44
        , space = VK_SPACE
    };
    
    using key_state_type = std::vector<input::key>;
    using signal_type = boost::signals2::signal<void (const key_state_type& key_state)>;

    struct down_signal{
        std::function<void(entt::registry&, entt::entity, const input::key_state_type&)> callback{};
    };
    struct up_signal : down_signal {};
    
    explicit input() = default;
    ~input() override;

    input(input& other) noexcept = delete;
    input(input&& other) noexcept = delete;
    input& operator=(const input& other) = delete;
    input& operator=(input&& other) noexcept = delete;

    
    void execute(entt::registry& reg) override
    {
        const input::key_state_type& state = get_down_key_state();
        // TODO слепок клавиатуры и убрать boost signal и receive input, только процессор
        for(const auto view = reg.view<const input::down_signal, const input::up_signal>();
            const auto entity: view){
            
            view.get<input::down_signal>(entity).callback(reg, entity, state);
            view.get<input::up_signal>(entity).callback(reg, entity, state);
            }
    }
    
    virtual void tick() override;
    void send_input();
    void receive(const LPMSG msg);
    [[nodiscard]] const key_state_type& get_down_key_state() const { return down_key_state_;}
    [[nodiscard]] input::signal_type& get_down_signal() {return on_down; }

private:
    
    signal_type on_down;
    signal_type on_up;
    key_state_type down_key_state_;
    key_state_type up_key_state_;

};


