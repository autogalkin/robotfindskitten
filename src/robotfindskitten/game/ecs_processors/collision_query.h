#pragma once
#ifndef _CPP_PROJECTS_ROBOTFINDSKITTEN_SRC_ROBOTFINDSKITTEN_GAME_ECS_PROCESSORS_COLLISION_QUERY_H
#define _CPP_PROJECTS_ROBOTFINDSKITTEN_SRC_ROBOTFINDSKITTEN_GAME_ECS_PROCESSORS_COLLISION_QUERY_H


#include <entt/entt.hpp>

#include "engine/details/base_types.hpp"
#include "engine/world.h"
#include "game/ecs_processors/collision.h"

namespace collision {

enum class responce { block, ignore };
struct agent {
private:
    friend class query;
    index index_in_quad_tree = invalid_index;
};

struct collider {
    entt::entity ent;
    explicit collider(const entt::entity& e): ent(e) {}
    explicit collider(entt::entity&& e): ent(e) {}
    // NOLINTNEXTLINE(google-explicit-constructor)
    operator entt::entity() const noexcept {
        return ent;
    }
};
struct self {
    entt::entity ent;
    explicit self(const entt::entity& e): ent(e) {}
    explicit self(entt::entity&& e): ent(e) {}
    // NOLINTNEXTLINE(google-explicit-constructor)
    operator entt::entity() const noexcept {
        return ent;
    }
};

struct on_collide {
    static responce block_always(entt::registry& /*unused*/, self /*unused*/,
                                 collider /*unused*/) {
        return responce::block;
    }
    static responce ignore_always(entt::registry& /*unused*/, self /*unused*/,
                                  collider /*unused*/) {
        return responce::ignore;
    }

    std::function<responce(entt::registry&, self s, collider cl)> call{
        &collision::on_collide::block_always};
};

class query: ecs_proc_tag {
public:
    explicit query(world& w, pos game_area);

    void execute(entt::registry& reg, timings::duration delta);

private:
    // mark entity to remove from tree and insert again
    struct need_update_entity {};
    // place outside of a class for store the processor in boost::any_container
    // and not resize a padding
    std::unique_ptr<quad_tree<entt::entity>> tree_;
};


query::query(world& w, const pos game_area)
    : tree_(std::make_unique<quad_tree>(box{{0, 0}, {game_area.x, game_area.y}},
                                        4)) {
    w.reg_.on_construct<collision::agent>()
        .connect<&entt::registry::emplace_or_replace<need_update_entity>>();
}

void query::execute(entt::registry& reg, timings::duration /*dt*/) {
    // insert moved actors into tree
    for(const auto view = reg.view<const loc, const sprite, need_update_entity,
                                   collision::agent, const visible_in_game>();
        const auto entity: view) {
        const auto& sh = view.get<sprite>(entity);
        const auto current_location = view.get<loc>(entity);

        view.get<collision::agent>(entity).index_in_quad_tree = tree_->insert(
            entity,
            box({0, 0}, sh.bounds()) + box(current_location, current_location));
        reg.remove<need_update_entity>(entity);
    }

    { // query
        const auto view =
            reg.view<const loc, translation, const sprite, collision::agent,
                     const collision::on_collide>();

        // compute collision
        for(const auto entity: view) {
            const loc& current_location = view.get<loc>(entity);
            translation& trans = view.get<translation>(entity);

            if(trans.get() == loc(0)) {
                continue; // checking only dynamic
            }

            std::deque<size_t> collides;
            tree_->query(box{{0, 0}, view.get<sprite>(entity).bounds()}
                             + box(current_location + trans.get(),
                                   current_location + trans.get()),
                         collides);
            if(!collides.empty()) {
                // TODO(Igor): on collide func
                //
                for(const auto collide: collides) {
                    if(const auto cl = tree_->get_entity(collide).id;
                       cl != entity) {
                        const auto cl_resp = view.get<on_collide>(cl).call(
                            reg, self(cl), collider(entity));
                        if(const auto resp = view.get<on_collide>(entity).call(
                               reg, self(entity), collider(cl));
                           cl_resp == responce::block
                           && resp == responce::block) {
                            trans = loc(0);
                            view.get<translation>(cl).mark();
                        }
                    }
                }
            }
        }
        // remove actors which will move, insert it again in the next tick
        for(const auto entity: view) {
            if(view.get<translation>(entity).get() != loc(0)
               || reg.all_of<life::begin_die>(entity)) {
                auto& [index_in_quad_tree] = view.get<collision::agent>(entity);
                tree_->remove(index_in_quad_tree);
                index_in_quad_tree = collision::invalid_index;
                reg.emplace_or_replace<need_update_entity>(entity);
            }
        }
    }
    tree_->cleanup();
}


} // namespace collision

#endif
