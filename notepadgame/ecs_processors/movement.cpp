#include "movement.h"
#include "range/v3/view/enumerate.hpp"
#include "range/v3/view/filter.hpp"
#include "../core/world.h"

void movement::execute(entt::registry& reg)
{
    
    for(const auto view = reg.view<location, const shape, velocity>();
        const auto entity: view)
    {
            
        auto& vel = view.get<velocity>(entity);
        //if(vel.isZero()) continue;

        const auto& [sh] = view.get<shape>(entity);
        auto& loc = view.get<location>(entity);

        auto des_loc = location{loc.line() + vel.line(), loc.index_in_line() + vel.index_in_line()};
        vel = velocity::null();
        // draw
        for(auto rows = sh.rowwise();
            auto [line, row] : rows | ranges::views::enumerate)
        {
            for(int ind_in_line{-1}; const auto ch : row
                | ranges::views::filter([&ind_in_line](const char c){++ind_in_line; return c != shape::whitespace;}))
            {
                get_world()->at(location(des_loc + location{static_cast<npi_t>(line), ind_in_line})) = ch;
                if(des_loc != loc) get_world()->at(loc) = shape::whitespace;
            }
        }
        loc = des_loc;
    }
}
