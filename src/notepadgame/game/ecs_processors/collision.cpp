#include "game/ecs_processors/collision.h"
#include "engine/notepad.h"
#include <glm/common.hpp>
#include <queue>
#include "game/ecs_processors/life.h"
#include "game/comps.h"
#include "engine/details/base_types.hpp"
#include "game/ecs_processors/motion.h"

namespace collision {

[[nodiscard]] bool intersect(const box a, const box b) {
    using namespace pos_declaration;
    return a[S][Y] < b[E][Y] && a[E][Y] > b[S][Y] && a[S][X] < b[E][X]
           && a[E][X] > b[S][X];
}
[[nodiscard]] pos center(const box b) {
    using namespace pos_declaration;
    return {b[S][Y] + b[E][Y] / 2, b[S][X] + b[E][X] / 2};
}
index quad_tree::insert(const id_type id, const box& bbox) {
    const quad_entity new_entity = {bbox, id};
    const index element = entities_.insert(new_entity);
    node_insert(root_data(), element);
    return element;
}

void quad_tree::remove(const size_t indx) {
    // For each leaf node, remove the element node.
    for(const auto leaves = find_leaves(root_data(), entities_[indx].bbox);
        const auto& nd: leaves) {
        quadnode& node = nodes_[nd.i];

        // Walk the list until we find the element node.
        index* link = &node.first_child;
        while(*link != end_of_list
              && entity_nodes_[*link].entity_index != indx) {
            link = &entity_nodes_[*link].next;
            assert(*link != end_of_list);
        }

        if(*link != end_of_list) {
            // Remove the element node.
            const index elt_node_index = *link;
            *link = entity_nodes_[*link].next;
            entity_nodes_.erase(elt_node_index);
            --node.count_entities;
        }
    }
    // Remove the element.
    entities_.erase(indx);
}

void quad_tree::query(const box& rect, std::vector<size_t>& intersect_result) {
    // TODO(Igor) ab и ba?
    /*
            for each leaf in leaves:
            {
                 for each element in leaf:
                 {
                      if not traversed[element]:
                      {
                          use quad tree to check for collision against other
       elements traversed[element] = true
                      }
                 }
            }
         */
    std::vector<bool> traversed(entities_.size(), false);
    // TODO(Igor) сам с собой сейчас коллайдится
    // Find the leaves that intersect the specified query rectangle.
    // For each leaf node, look for elements that intersect.
    for(const auto leaves = find_leaves(root_data(), rect);
        const auto& leaf: leaves) {
        const quadnode& node = nodes_[leaf.i];

        // Walk the list and add elements that intersect.
        auto elt_node_index = node.first_child;
        while(elt_node_index != end_of_list) {
            if(const size_t element =
                   entity_nodes_[elt_node_index].entity_index;
               !traversed[element]
               && intersect(rect, entities_[element].bbox)) {
                intersect_result.push_back(element);
                traversed[element] = true;
            }
            elt_node_index = entity_nodes_[elt_node_index].next;
        }
    }
}
// если рут подается то рекурсивно проходится по всем нодам
void quad_tree::traverse(
    const quad_node_data& start, const box& start_rect,
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    const std::function<void(quad_node_data&)>& branch_visitor,
    const std::function<void(quad_node_data&)>& leaf_visitor) const {
    std::queue<quad_node_data> to_process;
    to_process.push(start);

    while(!to_process.empty()) {
        quad_node_data nd = to_process.back();
        to_process.pop();
        if(const quadnode& node = nodes_[nd.i]; node.isleaf()) {
            leaf_visitor(nd);
        } else {
            branch_visitor(nd);
            // push the children that intersect the rectangle.
            if(auto quadrant = get_quadrant(nd.rect, start_rect);
               quadrant != quadrant::invalid_quadrant) {
                to_process.push(quad_node_data{
                    compute_child_box(nd.rect, quadrant),
                    node.first_child + static_cast<index>(quadrant),
                    nd.depth + 1});
            }
        }
    }
}

void quad_tree::cleanup() {
    // Only process the root if it's not a leaf.
    std::vector<size_t> to_process;
    if(!nodes_[0].isleaf()) {
        to_process.push_back(0);
    }

    while(!to_process.empty()) {
        const size_t node_index = to_process.back();
        to_process.pop_back();
        quadnode& node = nodes_[node_index];

        // Loop through the children.
        int num_empty_leaves = 0;
        for(const size_t child_index:
            ranges::views::iota(node.first_child, node.first_child + 4)) {
            const quadnode& child = nodes_[child_index];
            // Increment empty leaf count if the child is an empty leaf.
            if(child.count_entities == 0) {
                ++num_empty_leaves;
                // if the child is a branch, add it to
                //  the stack to be processed in the next iteration.
            } else if(child.count_entities == is_branch) {
                to_process.push_back(child_index);
            }
        }

        // If all the children were empty leaves, remove them and
        // make this node the new empty leaf.
        if(num_empty_leaves == 4) {
            merge(node);
        }
    }
}

void quad_tree::merge(quadnode& node) {
    // Push all 4 children to the free list.
    nodes_[node.first_child].first_child = free_node_;
    free_node_ = node.first_child;

    // Make this node the new empty leaf.
    node.first_child = end_of_list;
    node.count_entities = 0;
}

void quad_tree::node_insert(const quad_node_data& node_data,
                            const size_t element) {
    // Find the leaves and insert the element to all the leaves found.
    for(const std::vector<quad_node_data> leaves =
            find_leaves(node_data, entities_[element].bbox);
        const auto& leaf: leaves) {
        leaf_insert(leaf, element);
    }
}

void quad_tree::leaf_insert(const quad_node_data& leaf, const size_t element) {
    quadnode* node = &nodes_[leaf.i];

    // Insert the element node to the leaf.
    const entity_quad_node new_elt_node = {node->first_child, element};
    node->first_child = entity_nodes_.insert(new_elt_node);

    // If the leaf is full, split it.
    if(node->count_entities == max_elements && leaf.depth < max_depth) {
        split(*node, leaf);
    } else {
        ++node->count_entities;
    }
}

void quad_tree::split(quadnode& node, const quad_node_data& leaf) {
    assert(node.isleaf() && "Only leaves can be split");
    // Pop off all the previous elements.
    std::vector<size_t> elts;

    while(node.first_child != end_of_list) {
        const index index = node.first_child;
        node.first_child = entity_nodes_[node.first_child].next;
        elts.push_back(entity_nodes_[index].entity_index);
        entity_nodes_.erase(index);
    }

    // Start by allocating 4 child nodes.
    if(node.first_child != end_of_list) {
        free_node_ = nodes_[free_node_].first_child;
    } else {
        node.first_child = static_cast<int>(nodes_.size());
        nodes_.resize(nodes_.size() + 4);
    }

    // Initialize the new child nodes.
    for(int j = 0; j < 4; ++j) {
        nodes_[node.first_child + j].first_child = end_of_list;
        nodes_[node.first_child + j].count_entities = 0;
    }

    // Transfer the elements in the former leaf node to its new children.
    node.count_entities = is_branch;
    for(const auto& elt: elts) {
        node_insert(leaf, elt);
    }
}

quad_tree::quadrant
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
quad_tree::get_quadrant(const box& node_box, const box& value_box) {
    using namespace pos_declaration;
    auto center_pos = center(node_box);
    if(value_box[E][X] <= center_pos[X]) {
        if(value_box[S][Y] >= center_pos[Y]) {
            return quadrant::top_left;
        }
        if(value_box[S][Y] <= center_pos[Y]) {
            return quadrant::bottom_left;
        }
        return quadrant::invalid_quadrant;
    }
    if(value_box[S][X] >= center_pos[X]) {
        if(value_box[E][Y] <= center_pos[Y]) {
            return quadrant::top_right;
        }
        if(value_box[S][Y] >= center_pos[Y]) {
            return quadrant::bottom_right;
        }

        return quadrant::invalid_quadrant;
    }
    return quadrant::invalid_quadrant;
}

box quad_tree::compute_child_box(const box& parent, const quadrant child) {
    using namespace pos_declaration;
    pos child_size = parent[E] / pos(2);
    switch(child) {
    case quadrant::top_left:
        return {parent[S], child_size};
    case quadrant::top_right:
        return {{parent[S][Y], parent[S][X] + child_size[X]}, child_size};
    case quadrant::bottom_left:
        return {{parent[S][Y] + child_size[Y], parent[S][X]}, child_size};
    case quadrant::bottom_right:
        return {parent[S] + child_size, child_size};
    case quadrant::invalid_quadrant:
        break;
    }
    assert(false && "Invalid child quadrant");
    return {};
}

std::vector<quad_tree::quad_node_data>
quad_tree::find_leaves(const quad_node_data& start,
                       const box& start_rect) const {
    std::vector<quad_node_data> leaves;
    traverse(
        start, start_rect, [](quad_node_data&) {},
        [&leaves](const quad_node_data& nd) { leaves.push_back(nd); });
    return leaves;
}

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
                continue; // check only dynamic
            }

            std::vector<size_t> collides;
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
                        const auto cl_resp =
                            view.get<on_collide>(cl).call(reg, cl, entity);
                        if(const auto resp = view.get<on_collide>(entity).call(
                               reg, entity, cl);
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
