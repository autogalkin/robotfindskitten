#pragma once
#ifndef _CPP_PROJECTS_ROBOTFINDSKITTEN_SRC_ROBOTFINDSKITTEN_GAME_ECS_PROCESSORS_COLLISION_QUERY_H
#define _CPP_PROJECTS_ROBOTFINDSKITTEN_SRC_ROBOTFINDSKITTEN_GAME_ECS_PROCESSORS_COLLISION_QUERY_H

#include <entt/entt.hpp>

#include "engine/details/base_types.hpp"
#include "engine/world.h"
#include "game/systems/collision.h"
#include "game/systems/motion.h"
#include "game/systems/life.h"
#include "game/comps.h"

namespace collision {

class agent {
    friend class query;
    quad_tree<entt::entity>::inserted index_in_quad_tree =
        quad_tree<entt::entity>::inserted::null;
};

struct collider {
    entt::entity ent;
    // explicit collider(const entt::entity& e): ent(e) {}
    // explicit collider(entt::entity&& e): ent(e) {}
    // // NOLINTNEXTLINE(google-explicit-constructor)
    operator entt::entity() const noexcept {
        return ent;
    }
};
struct self {
    entt::entity ent;
    // explicit self(const entt::entity& e): ent(e) {}
    // explicit self(entt::entity&& e): ent(e) {}
    // // NOLINTNEXTLINE(google-explicit-constructor)
    operator entt::entity() const noexcept {
        return ent;
    }
};

// struct on_collide {
//     static responce block_always(entt::registry& /*unused*/, self /*unused*/,
//                                  collider /*unused*/) {
//         return responce::block;
//     }
//     static responce ignore_always(entt::registry& /*unused*/, self
//     /*unused*/,
//                                   collider /*unused*/) {
//         return responce::ignore;
//     }
//
//     std::function<responce(entt::registry&, self s, collider cl)> call{
//         &collision::on_collide::block_always};
// };

struct responce {
    entt::delegate<void(entt::registry&, self, collider)> fun;
    void operator()(entt::registry& reg, self s, collider c) {
        fun(reg, s, c);
    }
};

struct hit_box {
    box_t v;
};

class query: is_system {
    // mark entity to remove from tree and insert again
    struct need_update_entity {};
    quad_tree<entt::entity> tree_;

public:
    inline query(entt::registry& reg, const pos game_area)
        : tree_(box_t{{0, 0}, {game_area.x, game_area.y}}) {
        reg.on_construct<collision::agent>()
            .connect<&entt::registry::emplace_or_replace<
                query::need_update_entity>>();
    }

    inline void operator()(
        entt::view<entt::get_t<const loc, const hit_box, need_update_entity,
                               agent, const visible_tag>>
            need_insert,
        entt::view<entt::get_t<const loc, const translation, const hit_box,
                               const agent, const visible_tag>>
            query_view,
        entt::view<entt::get_t<const agent, const responce>> responce_view,
        entt::view<entt::get_t<const translation, agent,
                               entt::exclude_t<life::begin_die>>>
            remove,
        entt::view<entt::get_t<const life::begin_die, agent>> remove2,

        entt::registry& reg) {
        // insert moved actors into the tree
        for(const auto ent: need_insert) {
            const auto cur_loc = view.get<loc>(ent);
            auto& [index_in_quad_tree] = need_insert.get<agent>(ent);
            assert(index_in_quad_tree == decltype(tree_)::inserted::null);
            index_in_quad_tree =
                tree_.insert(entity, box_t({0, 0}, hit_box.v - pos(1))
                                         + box_t(cur_loc, cur_loc));
            reg.remove<need_update_entity>(ent); // safe, delete on the same ent
        }

        struct node {
            entt::entity ent;
            enum type { requester, collider };
            type ty;
        };
        std::deque<collision_node> result_list{};

        // compute collision
        for(const auto ent: query_view) {
            const loc& current_location = query_view.get<loc>(enty);
            translation& trans = query_view.get<translation>(ent);

            if(trans.get() == loc(0)) {
                continue; // checking only dynamics
            }
            result_list.push_back(node{.ent = ent, .type = node::requester});

            auto next_ent_box =
                box_t({0, 0}, query_view.get<hit_box>(entity).v - pos(1))
                + box_t(current_location + trans.get(),
                        current_location + trans.get());

            tree_.query({ent, next_ent_box}, [&result_list](entt::entity resp) {
                result_list.push_back(
                    collision_node{.ent = resp, .type = node::collider});
            });
        }
        if(!result_list.empty()) {
            auto curr_req = result_list[0];
            for(size_t i = 0; i < result_list.size();) {
                curr_req = i;
                ++i;
                const auto* const cur_req_fun =
                    responce_view.contains(curr_req.ent)
                        ? &responce_view.get<const responce>(n.ent)
                        : nullptr;
                while(result_list[i].ent == node::type::collider
                      && i < result_list.size()) {
                    if(cur_req_fun) {
                        (*cur_req_fun)(reg, self(curr_req),
                                       collider(result_list[i].ent));
                    }
                    if(responce_view.contains(result_list[i].ent)) {
                        responce_view.get<const responce>(n.ent)(
                            reg, self(result_list[i].ent), collider(curr_req));
                    }
                    ++i;
                }
            }
        }
        auto remove_from_tree = [&reg](auto& view, auto ent) {
            tree_.remove(remove2.get<agent>(ent).index_in_quad_tree);
            reg.emplace_or_replace<need_update_entity>(ent);
        };
        // remove actors which will move, insert it again in the next tick
        for(const auto ent: remove) {
            if(remove.get<translation>(ent).is_changed()) {
                remove_from_tree(remove, ent);
            }
        }
        for(const auto ent: remove2) {
            remove_from_tree(remove, ent);
        }
        tree_->cleanup();
    }
};

} // namespace collision

#endif
