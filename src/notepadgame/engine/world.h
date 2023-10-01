#pragma once
#include "details/nonconstructors.h"
#include <boost/type_erasure/relaxed.hpp>
#include <boost/type_erasure/operators.hpp>
#include <vector>
#include "engine/details/base_types.hpp"

namespace bte = boost::type_erasure;
template<class T = bte::_self>
struct is_ecs_proc {
    static void apply(T& cont, entt::registry& reg,
                      const timings::duration dt) {
        cont.execute(reg, dt);
    }
};
namespace boost::type_erasure {
template<class C, class T, class Base>
struct concept_interface<is_ecs_proc<C>, Base, T>: Base {
    void execute(entt::registry& reg, const timings::duration dt) {
        call(is_ecs_proc<C>(), *this, reg, dt);
    }
};
} // namespace boost::type_erasure
  //
using ecs_proc_base = bte::any<
    boost::mpl::vector<bte::constructible<bte::_self(bte::_self&&)>,
                       bte::destructible<>, bte::relaxed, is_ecs_proc<>>>;

struct ecs_proc_tag {};

class world: public noncopyable, public nonmoveable {
public:
    world() = default;
    // insertion order is important!
    // boost::any_collection<ecs_processor_base> processors;
    std::vector<ecs_proc_base> procs;
    // TODO(Igor): private?
    entt::registry reg_;
    void tick(timings::duration delta) {
        for(auto& i: procs) {
            i.execute(reg_, delta);
        }
    }

    template<typename F>
        requires std::is_invocable_v<F, entt::registry&, const entt::entity>
    void spawn_actor(F&& for_add_components) {
        const auto entity = reg_.create();
        for_add_components(reg_, entity);
    }
};
