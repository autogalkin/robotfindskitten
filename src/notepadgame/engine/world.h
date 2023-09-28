#pragma once
#include "details/nonconstructors.h"
#include <boost/type_erasure/relaxed.hpp>
#include <boost/type_erasure/operators.hpp>
#include <vector>
#include "engine/details/base_types.h"

namespace bte = boost::type_erasure;
template <class T = bte::_self> struct is_ecs_processor {
    static void apply(T& cont, entt::registry& reg,
                      const timings::duration delta) {
        cont.execute(reg, delta);
    }
};
namespace boost {
namespace type_erasure {
template <class C, class T, class Base>
struct concept_interface<is_ecs_processor<C>, Base, T> : Base {
    void execute(entt::registry& reg, const timings::duration delta) {
        call(is_ecs_processor<C>(), *this, reg, delta);
    }
};
} // namespace type_erasure
} // namespace boost
  //
using ecs_processor_base = bte::any< boost::mpl::vector<
                    bte::constructible<bte::_self(bte::_self&&)>,
                    bte::destructible<>,
                    bte::relaxed,
                    is_ecs_processor<>>
                  >;

class world : public noncopyable, public nonmoveable {
  public:
    // insertion order is important!
    // boost::any_collection<ecs_processor_base> processors;
    std::vector<ecs_processor_base> processors;

    entt::registry reg_;
    void tick(timings::duration delta) {
        for (auto& i : processors) {
            i.execute(reg_, delta);
        }
    }

    template <typename F>
        requires std::is_invocable_v<F, entt::registry&, const entt::entity>
    void spawn_actor(F&& for_add_components) {
        const auto entity = reg_.create();
        for_add_components(reg_, entity);
    }
};
