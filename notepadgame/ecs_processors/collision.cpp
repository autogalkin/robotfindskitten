 #include "collision.h"
#include "../core/notepader.h"

collision::index collision::quad_tree::insert(const id_type id, const boundbox& bbox)
{
    const quad_entity new_entity = {id,  bbox};
    const index element = entities_.insert(new_entity);
    node_insert(root_data(), element);
    return element;
}

void collision::quad_tree::remove(const size_t indx)
{
        
    // For each leaf node, remove the element node.
    for (const auto leaves = find_leaves(root_data(), entities_[indx].bbox)
         ; const auto& nd : leaves)
    {
        quadnode& node = nodes_[nd.i];
 
        // Walk the list until we find the element node.
        index* link = &node.first_child;
        while (*link != end_of_list && entity_nodes_[*link].entity_index != indx)
        {
            link = &entity_nodes_[*link].next;
            assert(*link != end_of_list);
        }
 
        if (*link != end_of_list)
        {
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

void collision::quad_tree::query(const boundbox& rect, std::vector<size_t>& intersect_result)
{
    // TODO ab и ba?
    /*
            for each leaf in leaves:
            {
                 for each element in leaf:
                 {
                      if not traversed[element]:
                      {
                          use quad tree to check for collision against other elements
                          traversed[element] = true                  
                      }
                 }
            }
         */
    std::vector<bool> traversed(entities_.size(), false);
    // TODO сам с собой сейчас коллайдится
    // Find the leaves that intersect the specified query rectangle.
    // For each leaf node, look for elements that intersect.
    for (const auto leaves = find_leaves(root_data(), rect)
         ; const auto& leaf : leaves)
    {
        const quadnode& node = nodes_[leaf.i];
        
        // Walk the list and add elements that intersect.
        auto elt_node_index = node.first_child;
        while (elt_node_index != end_of_list)
        {
            if (const size_t element = entity_nodes_[elt_node_index].entity_index
                ; !traversed[element] && rect.intersects(entities_[element].bbox))
            {
                intersect_result.push_back(element);
                traversed[element] = true;
            }
            elt_node_index = entity_nodes_[elt_node_index].next;
        }
    }
}
// если рут подается то рекурсивно проходится по всем нодам
void collision::quad_tree::traverse(const quad_node_data& start, const boundbox& start_rect, const std::function<void(quad_node_data&)>& branch_visitor
    , const std::function<void(quad_node_data&)>& leaf_visitor) const
{
    std::queue<quad_node_data> to_process;
    to_process.push(start);
    
    while (!to_process.empty())
    {
        quad_node_data nd = to_process.back(); to_process.pop();
        if (const quadnode& node = nodes_[nd.i]
            ; node.isleaf()){
            leaf_visitor(nd);
            }
        else
        {
            branch_visitor(nd);
            //push the children that intersect the rectangle.
            if (auto quadrant = get_quadrant(nd.rect, start_rect)
                ; quadrant != quadrant::invalid_quadrant){
                to_process.push(quad_node_data{compute_child_box(nd.rect, quadrant), node.first_child + static_cast<index>(quadrant), nd.depth + 1}); 
                }
            
        }
    }
}

void collision::quad_tree::cleanup()
{
    // Only process the root if it's not a leaf.
    std::vector<size_t> to_process;
    if (!nodes_[0].isleaf())
        to_process.push_back(0);

    while (!to_process.empty())
    {
        const size_t node_index = to_process.back(); to_process.pop_back();
        quadnode& node = nodes_[node_index];

        // Loop through the children.
        int num_empty_leaves = 0;
        for (const size_t child_index : ranges::views::iota(node.first_child, node.first_child + 4))
        {
            const quadnode& child = nodes_[child_index];
            // Increment empty leaf count if the child is an empty leaf. 
            if (child.count_entities == 0)
                ++num_empty_leaves;
                //if the child is a branch, add it to
                // the stack to be processed in the next iteration.
            else if (child.count_entities == is_branch)
                to_process.push_back(child_index);
        }

        // If all the children were empty leaves, remove them and 
        // make this node the new empty leaf.
        if (num_empty_leaves == 4){
            merge(node);
        }
    }
}

void collision::quad_tree::merge(quadnode& node)
{
    // Push all 4 children to the free list.
    nodes_[node.first_child].first_child = free_node_;
    free_node_ = node.first_child;

    // Make this node the new empty leaf.
    node.first_child = end_of_list;
    node.count_entities = 0;
}

void collision::quad_tree::node_insert(const quad_node_data& node_data, const size_t element)
{
    // Find the leaves and insert the element to all the leaves found.
    for (const std::vector<quad_node_data> leaves = find_leaves(node_data, entities_[element].bbox)
         ; const auto& leaf : leaves)
        leaf_insert(leaf, element);
}

void collision::quad_tree::leaf_insert(const quad_node_data& leaf, const size_t element)
{
    quadnode* node = &nodes_[leaf.i];
 
    // Insert the element node to the leaf.
    const entity_quad_node new_elt_node = {node->first_child, element};
    node->first_child = entity_nodes_.insert(new_elt_node);
 
    // If the leaf is full, split it.
    if (node->count_entities == max_elements && leaf.depth < max_depth)
    {
        split(*node, leaf);
    }
    else
        ++node->count_entities;
}

void collision::quad_tree::split(quadnode& node, const quad_node_data& leaf)
{
    assert(node.isleaf() && "Only leaves can be split");
    // Pop off all the previous elements.
    std::vector<size_t> elts;
            
    while (node.first_child != end_of_list)
    {
        const index index = node.first_child;
        node.first_child = entity_nodes_[node.first_child].next; 
        elts.push_back(entity_nodes_[index].entity_index);
        entity_nodes_.erase(index);
    }
 
    // Start by allocating 4 child nodes.
    if (node.first_child != end_of_list)
        free_node_ = nodes_[free_node_].first_child;
    else
    {
        node.first_child = static_cast<int>(nodes_.size());
        nodes_.resize(nodes_.size() + 4);
    }
 
    // Initialize the new child nodes.
    for (int j=0; j < 4; ++j)
    {
        nodes_[node.first_child+j].first_child = end_of_list;
        nodes_[node.first_child+j].count_entities = 0;
    }
 
    // Transfer the elements in the former leaf node to its new children.
    node.count_entities = is_branch;
    for (const auto& elt : elts)
        node_insert(leaf, elt);
}

collision::quad_tree::quadrant collision::quad_tree::get_quadrant(const boundbox& node_box, const boundbox& value_box)
{
    if (auto center = node_box.center()
        ; value_box.end().index_in_line() <= center.index_in_line())
    {
        if (value_box.pivot.line() <= center.line())
            return quadrant::top_left;
        else if (value_box.pivot.line() >= center.line())
            return quadrant::bottom_left;
        else
            return quadrant::invalid_quadrant;
    }
    else if (value_box.pivot.index_in_line() >= center.index_in_line())
    {
        if (value_box.end().line() <= center.line())
            return quadrant::top_right;
        else if (value_box.pivot.line() >= center.line())
            return quadrant::bottom_right;
        else
            return quadrant::invalid_quadrant;
    }
    else
        return quadrant::invalid_quadrant;
}

boundbox collision::quad_tree::compute_child_box(const boundbox& parent, const quadrant child)
{
    position child_size = parent.size / 2;
    switch (child)
    {
    case quadrant::top_left:
        return {parent.pivot, child_size};
    case quadrant::top_right:
        return { {parent.pivot.line(), parent.pivot.index_in_line() + child_size.index_in_line()}, child_size};
    case quadrant::bottom_left:
        return {{parent.pivot.line() + child_size.line(), parent.pivot.index_in_line()}, child_size};
    case quadrant::bottom_right:
        return {parent.pivot + child_size, child_size};
    case quadrant::invalid_quadrant: break;
    }
    assert(false && "Invalid child quadrant");
    return {};
}

std::vector<collision::quad_tree::quad_node_data> collision::quad_tree::find_leaves(const quad_node_data& start,
    const boundbox& start_rect) const
{
    std::vector<quad_node_data> leaves;
    traverse(start, start_rect, [](quad_node_data&){}, [&leaves](const quad_node_data& nd){
        leaves.push_back(nd);
    });
    return leaves;
}

collision::query::query(world* w): ecs_processor{w}
{
    notepader::get().get_engine()->get_on_scroll_changed().connect([this](const position& new_scroll){
        on_scroll_changed(new_scroll);
    });
    w->get_registry().on_construct<collision::agent>().connect<&entt::registry::emplace_or_replace<need_update_entity>>();
}

void collision::query::execute(entt::registry& reg, gametime::duration delta)
{
    
    // insert moved actors into tree
    for(const auto view = reg.view<const location_buffer, const shape::sprite_animation, need_update_entity, collision::agent, const visible_in_game>()
        ; const auto entity : view){
        auto& sh = view.get<shape::sprite_animation>(entity);
        const auto& [current_location, translation] = view.get<location_buffer>(entity);
        view.get<collision::agent>(entity).index_in_quad_tree = tree.insert(entity, sh.current_sprite().bound_box()+position_converter::from_location(current_location));
        reg.remove<need_update_entity>(entity);
    }

    {// query 
        const auto view = reg.view<location_buffer, const shape::sprite_animation, collision::agent, const collision::on_collide>();
            
        // compute collision
        for(const auto entity : view)
        {
            
            auto& [current_location, translation] = view.get<location_buffer>(entity);

            if(translation.get().is_null()) continue; // check only dynamic
            
            std::vector<size_t> collides;
            tree.query(view.get<shape::sprite_animation>(entity).current_sprite().bound_box()+(position_converter::from_location(current_location+translation.get())), collides);
            if(!collides.empty()){
                // TODO on collide func
                //
                for (const auto collide : collides){
                    if(const auto cl = tree.get_entity(collide).id;
                        cl != entity)
                    {
                        const auto cl_resp = view.get<on_collide>(cl).call(reg, cl, entity);
                        if(const auto resp = view.get<on_collide>(entity).call(reg, entity, cl)
                            ; cl_resp == responce::block && resp == responce::block){
                            translation = location::null();
                            translation.mark_dirty();
                            view.get<location_buffer>(cl).translation.mark_dirty();  
                        }
                        
                    }
                }
            }
        }
        // remove actors which will move, insert it again in the next tick 
        for(const auto entity : view)
        {
            if(!view.get<location_buffer>(entity).translation.get().is_null() || reg.all_of<life::begin_die>(entity)){
                auto& [index_in_quad_tree] = view.get<collision::agent>(entity);
                tree.remove(index_in_quad_tree);
                index_in_quad_tree = collision::invalid_index;
                reg.emplace_or_replace<need_update_entity>(entity);
            }
        }
    } 
    tree.cleanup();
}

