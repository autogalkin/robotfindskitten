#pragma once
#include "details/nonconstructors.h"
#include <stdint.h>
#include <type_traits>
#include <boost/poly_collection/any_collection.hpp>
#include <boost/type_erasure/operators.hpp>

#include "df/dirtyflag.h"
#include <memory>
#include <vector>

#include "engine/details/base_types.h"
#include "engine/ecs_processor_base.h"
#include "engine/time.h"
#include "engine/buffer.h"


template <class T=boost::type_erasure::_self>  struct is_ecs_processor {
  static void apply(T &cont, entt::registry& reg, const timings::duration delta) { cont.execute(reg, delta); }
};
namespace boost {
namespace type_erasure {
template <class C, class T, class Base>
struct concept_interface<is_ecs_processor<C>, Base, T> : Base {
  void execute(entt::registry& reg, const timings::duration delta) { call(is_ecs_processor<C>(), *this, reg, delta); }
};
} // namespace type_erasure
} // namespace boost




class world: public noncopyable, public nonmoveable {
  public:
    world(back_buffer& buf) noexcept;
    ~world();

    boost::any_collection<is_processor<>> processors{};
    // TODO
    back_buffer& backbuffer;
    entt::registry reg_;
    void tick(timings::duration delta) {
        for (const auto& i : all) {
            i->execute(reg_, delta);
        }
    }

    template <typename F>
        requires std::is_invocable_v<F, entt::registry&, const entt::entity>
    void spawn_actor(F&& for_add_components) {
        const auto entity = reg_.create();
        for_add_components(reg_, entity);
    }

};
