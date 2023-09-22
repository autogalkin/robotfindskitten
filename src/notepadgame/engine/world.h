#pragma once
#include "details/nonconstructors.h"
#include <stdint.h>
#include <type_traits>
#pragma warning(push, 0)
#include "df/dirtyflag.h"
#include <memory>
#include <vector>
#pragma warning(pop)
#include "details/base_types.h"
#include "ecs_processor_base.h"
#include "tick.h"
#include "buffer.h"

class ecs_processors_executor {
  public:
    enum class insert_order : int8_t { before = 0, after = 1 };
    bool insert_processor_at(
        std::unique_ptr<ecs_processor> who,
        const std::type_info& near_with, /* usage: typeid(ecs_processor) */
        const insert_order where = insert_order::before);
    void execute(entt::registry& reg, const gametime::duration delta) const {
        for (const auto& i : data_) {
            i->execute(reg, delta);
        }
    }

    template <typename T, typename... Args>
        requires std::derived_from<T, ecs_processor> &&
                 std::is_constructible_v<T, Args...>
    void push(Args&&... args) {
        data_.emplace_back(std::make_unique<T>(std::forward<Args>(args)...));
    }

  private:
    std::vector<std::unique_ptr<ecs_processor>> data_;
};

class world final : public tickable_base { //, nonmoveable, noncopyable {
  public:
    world(back_buffer* buf, ticker::signal_t& on_tick) noexcept;
    ~world() override;

    ecs_processors_executor executor;
    // TODO
    back_buffer* backbuffer;
    entt::registry reg_;
    void tick(gametime::duration delta) override {
        executor.execute(reg_, delta);
    }

    template <typename F>
        requires std::is_invocable_v<F, entt::registry&, const entt::entity>
    void spawn_actor(F&& for_add_components) {
        const auto entity = reg_.create();
        for_add_components(reg_, entity);
    }

  private:
    void redraw_all_actors();
    /// boost::signals2::scoped_connection scroll_changed_connection_;
    // boost::signals2::scoped_connection size_changed_connection;
};
