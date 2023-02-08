#pragma once
#include <chrono>
#include "../core/base_types.h"
#include "../core/tick.h"

/*
struct lifetime
{
    std::chrono::milliseconds value{};
    
};

class life final : public ecs_processor
{
public:
    void execute(entt::registry& reg) override
    {
        using namespace std::chrono_literals;
        for(const auto view = reg.view<lifetime>();
            const auto entity: view)
        {
            auto& [lt] = view.get<lifetime>(entity);
            //lt -= ticker::clock::now() - last_time_;
            if(lt < 0ms)
            {
                reg.destroy(entity);
            }
        }
    }
private:
    ticker::time_point last_time_;
};
*/