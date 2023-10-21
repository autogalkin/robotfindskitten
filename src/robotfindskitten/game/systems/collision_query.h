/**
 * @file
 * @brief The Quad Tree collision detection ECS system.
 */

#ifndef _CPP_PROJECTS_ROBOTFINDSKITTEN_SRC_ROBOTFINDSKITTEN_GAME_ECS_PROCESSORS_COLLISION_QUERY_H
#define _CPP_PROJECTS_ROBOTFINDSKITTEN_SRC_ROBOTFINDSKITTEN_GAME_ECS_PROCESSORS_COLLISION_QUERY_H

#include <entt/entity/entity.hpp>
#include <entt/entity/registry.hpp>

#include "engine/details/base_types.hpp"
#include "game/systems/collision.h"
#include "game/systems/motion.h"
#include "game/systems/life.h"
#include "game/systems/drawers.h"

namespace collision {

/**
 * @brief An index inside \ref collision::quad_tree
 */
class agent {
    friend class query;
    quad_tree<entt::entity>::inserted index_in_quad_tree{};
};

/**
 * @brief A transparency type to avoid swappable mistakes with requester
 * entity
 */
struct collider {
    entt::entity ent;
    // // NOLINTNEXTLINE(google-explicit-constructor)
    operator entt::entity() const noexcept {
        return ent;
    }
};
/**
 * @brief A transparency type to avoid swappable mistakes with colliders
 * entity
 */
struct self {
    entt::entity ent;
    // // NOLINTNEXTLINE(google-explicit-constructor)
    operator entt::entity() const noexcept {
        return ent;
    }
};

/**
 * @brief A collision callback
 */
struct responce_func {
    using function_type = entt::delegate<void(entt::registry&, self, collider)>;
    function_type fun;
    void operator()(entt::registry& reg, self s, collider c) const {
        fun(reg, s, c);
    }
};

/**
 * @brief A hit box of the entity in the game
 */
struct hit_extends {
    pos v;
};

/**
 * @brief Quad Tree collision detection system
 */
class query {
    // Marking the entity for removal from the tree and insertion again in the
    // next tick.
    struct need_update_entity {};
    quad_tree<entt::entity> tree_;

public:
    inline query(entt::registry& reg, const pos game_area)
        : tree_(box_t{{0, 0}, {game_area.x, game_area.y}}) {
        reg.on_construct<collision::agent>()
            .connect<&entt::registry::emplace_or_replace<
                query::need_update_entity>>();
    }

    inline void operator()( // NOLINT(readability-function-cognitive-complexity)
        const entt::view<
            entt::get_t<const loc, const hit_extends, const need_update_entity,
                        agent, const drawing::visible_tag>>& need_insert,
        const entt::view<
            entt::get_t<const loc, const translation, const hit_extends,
                        const agent, const drawing::visible_tag>>& query_view,
        const entt::view<entt::get_t<const agent, const responce_func>>&
            responce_view,
        const entt::view<entt::get_t<const translation, agent>,
                         entt::exclude_t<life::begin_die>>& remove_by_move,
        const entt::view<entt::get_t<const life::begin_die, agent>>&
            remove_by_die,

        entt::registry& reg) {
        // Insert moved actors into the tree
        for(const auto ent: need_insert) {
            const auto cur_loc = need_insert.get<loc>(ent);
            auto& [index_in_quad_tree] = need_insert.get<agent>(ent);
            assert(!index_in_quad_tree.is_valid());
            index_in_quad_tree = tree_.insert(
                ent,
                box_t({0, 0}, need_insert.get<hit_extends>(ent).v - pos(1)) //
                    + box_t(cur_loc, cur_loc));
            reg.erase<need_update_entity>(ent); // safe, delete on the same ent
        }
        // Pack intersections into a flat buffer
        struct node {
            entt::entity ent;
            enum type { requester, collider } ty;
        };
        std::deque<node> result_list{};

        // Compute intersections
        for(const auto ent: query_view) {
            const loc& current_location = query_view.get<loc>(ent);
            const translation& trans = query_view.get<translation>(ent);

            if(trans.get() == loc(0)) {
                continue; // checking only dynamics
            }
            result_list.push_back(node{.ent = ent, .ty = node::requester});
            auto next_ent_box =
                box_t({0, 0}, query_view.get<hit_extends>(ent).v - pos(1))
                + box_t(current_location + trans.get(),
                        current_location + trans.get());

            tree_.query({ent, next_ent_box}, [&result_list](entt::entity resp) {
                result_list.push_back(node{.ent = resp, .ty = node::collider});
            });
        }
        // Execute callbacks
        if(!result_list.empty()) {
            size_t curr_req = 0;
            for(size_t i = 0; i < result_list.size();) {
                curr_req = i;
                ++i;
                const auto* const cur_req_fun =
                    responce_view.contains(result_list[curr_req].ent)
                        ? &responce_view.get<const responce_func>(
                            result_list[curr_req].ent)
                        : nullptr;
                while(i < result_list.size()
                      && result_list[i].ty == node::type::collider) {
                    if(cur_req_fun) {
                        (*cur_req_fun)(reg, self{result_list[curr_req].ent},
                                       collider{result_list[i].ent});
                    }
                    if(responce_view.contains(result_list[i].ent)) {
                        responce_view.get<const responce_func>(
                            result_list[i].ent)(
                            reg, self{result_list[i].ent},
                            collider{result_list[curr_req].ent});
                    }
                    ++i;
                }
            }
        }
        auto remove_from_tree = [&reg, this](auto& view, auto ent) {
            tree_.remove(view.template get<agent>(ent).index_in_quad_tree);
            reg.emplace_or_replace<need_update_entity>(ent);
        };
        // Remove actors which will move, insert them again in the next tick
        for(const auto ent: remove_by_move) {
            if(remove_by_move.get<translation>(ent).is_changed()) {
                remove_from_tree(remove_by_move, ent);
            }
        }
        for(const auto ent: remove_by_die) {
            remove_from_tree(remove_by_move, ent);
        }
        tree_.cleanup();
    }
};

} // namespace collision

#endif
