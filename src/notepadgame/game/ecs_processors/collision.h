#pragma once
#pragma warning(push, 0)
#include <chrono>
#include <entt/entt.hpp>
#include "range/v3/view/iota.hpp"
#include "boost/container/small_vector.hpp"
#include <variant>
#pragma warning(pop)

#include "details/base_types.h"
#include "details/gamelog.h"


namespace collision
{
    class query;
    using index = int64_t;
    inline static constexpr int end_of_list   = -1;
    inline static constexpr int is_branch     = -1;
    inline static constexpr int invalid_index = -1;

    template <typename T>
    class object_pool
    {
    public:
        using next_free_index = index;
        //using data_type = boost::container::small_vector<std::variant<T, next_free_index>, 128 >;
        using data_type = std::vector<std::variant<T, next_free_index>>;
        index insert(const T& elt){
            if (first_free_ != end_of_list){
                const index ind = first_free_;
                first_free_ = std::get<next_free_index>(data_[first_free_]);
                data_[ind] = elt;
                return ind;
            }
            else{
                data_.push_back(std::variant<T, next_free_index>{elt});
                return static_cast<index>(data_.size() - 1);
            }
        }
        
        void erase(size_t n){
            data_[n] = first_free_;
            first_free_ = static_cast<index>(n);
        }
        
        void clear(){
            this->clear();
            first_free_ = end_of_list;
        }
       
        T& operator[](size_t n){
            return  std::get<T>(data_[n]);
        }
        const T& operator[](size_t n) const{
            return std::get<T>(data_[n]);
        }
        size_t size() {return data_.size();}
    private:
        
        index first_free_ = end_of_list;
        data_type data_{};
    };
    
    class quad_tree
    {
        friend class collision::query;
    public:
        using id_type = entt::entity;
        // is a max number of values a node can contain before we try to split it
        static constexpr std::size_t max_elements = 8;
        
        // represent an entity in the quadtree.
        //An element is only inserted once to the quadtree no matter how many cells it occupies.
        struct quad_entity{
            boundbox bbox;
            id_type id{};
            
        };
        
    private:
        struct quadnode
        {
            // Points to the first child if this node is a branch or the first element
            // if this node is a leaf.
            index first_child = end_of_list;
            // Stores the number of elements in the node or -1 if it is not a leaf.
            int count_entities = is_branch;
            [[nodiscard]] bool isleaf() const{
                return count_entities != is_branch;
            }
        };
        enum class quadrant : int8_t
        {
            top_left     = 0
          , top_right    = 1
          , bottom_left  = 2
          , bottom_right = 3
          , invalid_quadrant = -1
        };
        //for each cell it occupies, an "element node" is inserted which indexes that element.
        //singly-linked index list node into an array
        struct entity_quad_node{
            // Points to the next element in the leaf node. 
            index next = end_of_list;
            size_t entity_index = 0;
        };
        // Stores temporary data about a node during a search.
        struct quad_node_data{
            boundbox rect{};
            index i = 0;
            int depth = 0;
        };
        object_pool<quad_entity> entities_;
        object_pool<entity_quad_node> entity_nodes_;
        // Stores all the nodes in the quadtree. The first node in this sequence is always the root.
        // boost::container::small_vector<quadnode, 128>
        std::vector<quadnode> nodes_;
        // Stores the quadtree extents.
        boundbox root_rect_;
        // Stores the first free node in the quadtree to be reclaimed as 4
        // contiguous nodes at once. A value of -1(end_of_list) indicates that the free
        // list is empty, at which point we simply insert 4 nodes to the
        // back of the nodes array.
        index free_node_ = end_of_list;
        // Stores the maximum depth allowed for the quadtree.
        uint8_t max_depth;
        
    public:
        
        explicit quad_tree(boundbox rootrect, const uint8_t max_depth = 8): 
        root_rect_(std::move(rootrect)), free_node_(end_of_list), max_depth(max_depth){
            constexpr quadnode root_node = {.first_child = end_of_list, .count_entities =0};
            nodes_.reserve(128);
            nodes_.push_back(root_node);
        }

        [[nodiscard]] const boundbox& get_root_rect() const {return root_rect_;}
        quad_entity& get_entity(const size_t element){
            return entities_[element];
        }
        
        index insert(const id_type id, const boundbox& bbox);

        void remove(const size_t indx);

        void query(const boundbox& rect, std::vector<size_t>& intersect_result);

        // delete empty leaves
        void cleanup();

        void traverse(const quad_node_data& start, const boundbox& start_rect, const std::function<void(quad_node_data&)>& branch_visitor
            , const std::function<void(quad_node_data&)>& leaf_visitor) const;

    private:
        
        [[nodiscard]] quad_node_data root_data() const{
            return {.rect = root_rect_, .i = 0, .depth = 0};
        }
        
        void node_insert(const quad_node_data& node_data, const size_t element);

        void leaf_insert(const quad_node_data& leaf, const size_t element);
        
        void split(quadnode& node, const quad_node_data& leaf);

        // the quadrant in which a value is
        static quadrant get_quadrant(const boundbox& node_box, const boundbox& value_box);

        // compute from the box of parent node and index of its quadrant
        static boundbox compute_child_box(const boundbox& parent, const quadrant child);

        [[nodiscard]] std::vector<quad_node_data> find_leaves(const quad_node_data& start, const boundbox& start_rect) const;

        void merge(quadnode& node);
    };

    enum class responce
    {
          block
        , ignore
    };
    struct agent{
    private:
        friend class query;
        index index_in_quad_tree = invalid_index;
    };
    
    struct on_collide{
        static  responce block_always(entt::registry&, const entt::entity, const entt::entity) { return responce::block;}
        static  responce ignore_always(entt::registry&, const entt::entity, const entt::entity){ return responce::ignore;}
        
        std::function<responce(entt::registry&, entt::entity self,  entt::entity collider)> call{&collision::on_collide::block_always};
    };

    
    class query final : public ecs_processor
    {
    public:
        explicit query(world* w);

        virtual void execute(entt::registry& reg, gametime::duration delta) override;
    private:
        
        // mark entity to remove from tree and insert again
        struct need_update_entity{};
        quad_tree tree{boundbox{{0, 0}, {1000, 1000}}, 4};

    };

}





