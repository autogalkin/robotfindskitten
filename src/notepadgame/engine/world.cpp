
#include "details/base_types.h"
#include "details/gamelog.h"
#include "df/dirtyflag.h"
#include "tick.h"
#include <algorithm>
#include <numeric>
#pragma warning(push, 0)
#include "range/v3/view/enumerate.hpp"
#include "range/v3/view/filter.hpp"
#include "utf8cpp/utf8.h"
#pragma warning(pop)
#include "engine.h"
#include "notepad.h"
#include "world.h"
#include <ranges>


world::world(back_buffer* buf, ticker::signal_t& on_tick) noexcept : tickable_base(on_tick), backbuffer(buf) {
    /*
    scroll_changed_connection_ = engine_->get_on_scroll_changed().connect(
        [this](const position_t& new_scroll) { redraw_all_actors(); });

    size_changed_connection = engine_->get_on_resize().connect(
        [this](uint32_t width, uint32_t height) { redraw_all_actors(); });
    */
}

world::~world() = default;

void world::redraw_all_actors() {
    for (const auto view = reg_.view<location_buffer>();
         const auto entity : view) {
        view.get<location_buffer>(entity).translation.mark();
    }
}
bool ecs_processors_executor::insert_processor_at(
    std::unique_ptr<ecs_processor> who,
    const std::type_info& near_with, /* typeid(ecs_processor) */
    const insert_order where) {
    if (auto it = std::ranges::find_if(
            data_,
            [&near_with](const std::unique_ptr<ecs_processor>& n) {
                return near_with == typeid(*n);
            });
        it != data_.end()) {
        switch (where) {
        case insert_order::before:
            break;
        case insert_order::after:
            ++it;
            break;
        }
        data_.insert(it, std::move(who));
        return true;
    }
    return false;
}
