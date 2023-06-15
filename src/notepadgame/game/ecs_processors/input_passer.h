#pragma once
#pragma warning(push, 0)
#include "Windows.h"
#include "boost/signals2.hpp"
#include <entt/entt.hpp>
#pragma warning(pop)

#include "input.h"
#include "details/base_types.h"
#include "ecs_processor_base.h"



class input_passer final : public ecs_processor
{
public:
    explicit input_passer(world* w, input* i): ecs_processor(w), input_(i){}

    struct down_signal{
        // for customize input in runtime
        boost::signals2::signal<void (entt::registry&, entt::entity, const input::key_state_type&)> call;
    };
    struct up_signal : down_signal {};
    
    void execute(entt::registry& reg, gametime::duration) override
    {
        const input::key_state_type& state = input_->get_down_key_state();
        if(state.empty()) return;
        
        for(const auto view = reg.view<const input_passer::down_signal>();
            const auto entity: view){
            
            view.get<input_passer::down_signal>(entity).call(reg, entity, state);
            //view.get<input_passer::up_signal>(entity).callback(reg, entity, state);
            }
    }
    input* input_{nullptr};
};