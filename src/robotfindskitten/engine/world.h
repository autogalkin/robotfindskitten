﻿/**
 * @file
 * @brief Define ECS processor interface and ECS processors storage
 */

#pragma once

#include <vector>

#include <boost/type_erasure/relaxed.hpp>
#include <boost/type_erasure/operators.hpp>
#include <entt/entt.hpp>

#include "engine/time.h"
#include "details/nonconstructors.h"

namespace bte = boost::type_erasure;

/**
 * @brief Type-erasure concept
 * for all ecs_processors for use in boost::type_erasure::any
 */
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

//  ECS processor type
using ecs_proc_base = bte::any<
    boost::mpl::vector<bte::constructible<bte::_self(bte::_self&&)>,
                       bte::destructible<>, bte::relaxed, is_ecs_proc<>>>;


/**
 * @class ecs_proc_tag
 * @brief Tag for mark a class as a ECS processor in the source code
 * Used for identify classes through grep
 */
struct ecs_proc_tag {};

/**
 * @class world
 * @brief ECS processors and ECS registry storage
 *
 */
class world: public noncopyable, public nonmoveable {
public:
    world() = default;
    // WARNING: insertion order is important! and
    // boost::any_collection<ecs_processor_base> processors can not be used here
    //
    // All Processors stored by insertion order, and calls
    // from 0 to n
    std::vector<ecs_proc_base> procs;

    // FIXME(Igor): private?
    //
    // Main ECS game registry
    entt::registry reg_;

    /**
     * @brief Execute all ecs processors in their insertion order
     *
     * @param delta time between last execution
     */
    void tick(timings::duration delta) {
        for(auto& i: procs) {
            i.execute(reg_, delta);
        }
    }

    /**
     * @brief Spawn a new entity
     *
     * @tparam F Functor for add components to entity
     * @param for_add_components an instance of F
     */
    template<typename F>
        requires std::is_invocable_v<F, entt::registry&, const entt::entity>
    void spawn_actor(F&& for_add_components) {
        const auto entity = reg_.create();
        for_add_components(reg_, entity);
    }
};
