/**
 * @file
 * @brief Define ECS processor interface and ECS processors storage
 */

#pragma once
#ifndef _CPP_PROJECTS_ROBOTFINDSKITTEN_SRC_ROBOTFINDSKITTEN_ENGINE_WORLD_H
#define _CPP_PROJECTS_ROBOTFINDSKITTEN_SRC_ROBOTFINDSKITTEN_ENGINE_WORLD_H

#include <vector>

#include <boost/type_erasure/operators.hpp>
#include <boost/type_erasure/relaxed.hpp>
#include <entt/entt.hpp>

#include "engine/time.h"

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

//  ECS processor type for boost type erasure
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
struct world{
    // NOTE: insertion order is important! and also we store always unique types
    // `boost::any_collection<ecs_processor_base>` cannot be used here
    //
    // All Processors stored by insertion order, and calls
    // from 0 to n
    std::vector<ecs_proc_base> procs;

    // Main ECS game registry
    entt::registry registry;

    /**
     * @brief Execute all ecs processors in their insertion order
     *
     * @param delta time between last execution
     */
    void tick(timings::duration delta) {
        for(auto& i: procs) {
            i.execute(registry, delta);
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
        const auto entity = registry.create();
        for_add_components(registry, entity);
    }
};

#endif
