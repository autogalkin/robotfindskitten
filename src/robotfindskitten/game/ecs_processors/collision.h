#pragma once
#ifndef _CPP_PROJECTS_ROBOTFINDSKITTEN_SRC_ROBOTFINDSKITTEN_GAME_ECS_PROCESSORS_COLLISION_H
#define _CPP_PROJECTS_ROBOTFINDSKITTEN_SRC_ROBOTFINDSKITTEN_GAME_ECS_PROCESSORS_COLLISION_H

#include <format>
#include <stack>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <variant>

#include <boost/container/small_vector.hpp>
#include <boost/dynamic_bitset.hpp>
#include <entt/entt.hpp>
#include <glm/mat2x2.hpp>

namespace collision {

// int64_t Is a signed type guaranteed to be at least 64 bits.
// it will almost certainly hold all relevant values of type size_t
// Needs for use a -1 marker
using indx_t = int64_t;
// Needs for mark some indexes as invalid and unused
static_assert(std::is_signed_v<indx_t>);

// The main component for perform collision queries
// Represent `Start` corner of the box and the `End` corner
using box_t = glm::mat<2, 2, indx_t>;
static_assert(std::is_trivially_copyable_v<box_t>);
// A half of a box
using point_t = glm::vec<2, indx_t>;

namespace details {
template <class T, class... Args>
concept ConvertibleTo = (std::is_convertible_v<T, Args> && ...);

} // namespace details

namespace free_list {
class bad_free_list_access : public std::runtime_error {
public:
  explicit bad_free_list_access(const std::string &what)
      : std::runtime_error{what} {}
  explicit bad_free_list_access(const char *what) : std::runtime_error{what} {}
};

/**
 * @brief An unordered dynamic container that support erace O(1) and insert O(1)
 *
 * Very poor implementation of Free List: a container that stores
 * union of value T and pointer to previos erased elements. Erased elements made
 * a linked list chain of vacant positions, value of index stores the index to
 * the next free ceil.
 */
template <typename Element> class free_list {
  // a value of the next empty index in the "empty indexes" linked list
  enum class next_free_index : indx_t {};

  // Some imroves for code-readability
  inline static constexpr next_free_index END_OF_LIST =
      static_cast<next_free_index>(-1);

  using value_type = std::variant<Element, next_free_index>;
  using container_t =
#ifdef NDEBUG
      boost::container::small_vector<value_type, 128>;
#else
      // for LLDB pretty printing
      std::vector<value_type>;
#endif // NDEBUG

private:
  // A tail of free indexes linked list
  next_free_index first_free_ = END_OF_LIST;
  container_t data_;

public:
  template <typename... ManyElements>
    requires details::ConvertibleTo<Element, ManyElements...>
  explicit free_list(ManyElements... elts) : data_{elts...} {}
  explicit free_list() = default;

  // NOTE: It can return a custom bidirectional iterator but
  // it is not useful in the quad_tree context, because inserted key must be
  // stable and not will be invalidated through potential reallocations of the
  // container_t

  /**
   * @brief Insert an element into the list
   *
   * @param elt An element to insert
   * @return Inserted element position in the list
   */
  template <typename... Args>
    requires std::is_convertible_v<Args..., Element>
  [[nodiscard]] indx_t emplace(Args &&...elt) {
    // if the list has already eraced elements,
    // it means his "empty indexes" linked list is not empty.
    // Find a first free position and insert into it.
    if (first_free_ != END_OF_LIST) {
      const auto ind = static_cast<indx_t>(std::exchange(
          first_free_,
          std::get<next_free_index>(data_[static_cast<indx_t>(first_free_)])));
      data_[ind].template emplace<Element>(std::forward<Args>(elt)...);
      return ind;
    }
    // else emplace back
    data_.emplace_back(std::forward<Args>(elt)...);
    return static_cast<indx_t>(data_.size() - 1);
  }

  /**
   * @brief Remove an element by index from the list
   *
   * @param n Index of the element to be remove
   */
  void erase(indx_t n) noexcept(false) {
    throw_if_invalid_index(n);
    data_[n] = std::exchange(first_free_, static_cast<next_free_index>(n));
  }

  /**
   * @brief Remove all elements from the list
   */
  void clear() noexcept {
    data_.clear();
    first_free_ = END_OF_LIST;
  }
  Element &operator[](size_t n) noexcept(false) {
    throw_if_invalid_index(n);
    return std::get<Element>(data_[n]);
  }
  const Element &operator[](size_t n) const noexcept(false) {
    throw_if_invalid_index(n);
    return std::get<Element>(data_[n]);
  }
  /**
   * @brief Count of all elements with invalid elements too
   * (type of the element in the free list
   * is std::variant<YourAliveElement, NextFreeIndex>)
   *
   * @return The size of the data structure storage
   */
  [[nodiscard]] size_t all_size() const noexcept { return data_.size(); }

private:
  void throw_if_invalid_index(indx_t n) const noexcept(false) {
    if (n == -1) {
      throw std::invalid_argument{"n is an INVALID_INDEX = -1"};
    }
    if (!(n < data_.size()) || n < 0) {
      throw std::out_of_range{std::format(
          "erase out of bounds of the free list of size = {}, n = {}",
          data_.size(), n)};
    }
    if (!std::holds_alternative<Element>(data_[n])) {
      throw bad_free_list_access{"Attempt to access to removed value"};
    }
  }
};
} // namespace free_list

namespace details {

namespace pos_declaration {
// represent a box in the quad tree

// clang-format off
/*
   S      X(index in line)
   +------+----------------------->  + SY
   |      |                          |
   |      |                          |
   |      |                          |
   +------+                          + EY
   |      E
   | 
   v
   Y (line)
   +------+
   SX     EX
*/
// clang-format on

// X - horizontal coordinates from left to right
inline static constexpr size_t X = 0;
// Y - vertical coordinates from top to bottom
inline static constexpr size_t Y = 1;

// Start point
inline static constexpr size_t S = 0;
// End point
inline static constexpr size_t E = 1;

// Quadrants type, represent each child index of 4 branch children
// int the frat array
enum class quadrant : int8_t {
  top_left = 0,
  top_right = 1,
  bottom_left = 2,
  bottom_right = 3
};

} // namespace pos_declaration

/**
 * @brief Check two 2D box for intersection
 *
 * @param a one 2D box
 * @param b another 2D box
 * @return intersect or not
 */
[[nodiscard]] inline bool intersect(const box_t a, const box_t b) noexcept {
  using namespace pos_declaration;
  //     S  ______  S  ______
  //       |      |   |      |
  //       |   a  |   |   b  |
  //       |______|   |______|
  //              E           E
  return a[S][X] <= b[E][X] && a[E][X] >= b[S][X] && a[S][Y] <= b[E][Y] &&
         a[E][Y] >= b[S][Y];
}

/**
 * @brief Get Center of the 2D box
 *
 * @param b box
 * @return center point
 */
[[nodiscard]] inline point_t center(const box_t b) noexcept {
  using namespace pos_declaration;
  return {(b[E][X] - b[S][X]) / 2, (b[E][Y] - b[S][Y]) / 2};
}

/**
 * @brief Get quadrants in which a value is
 *
 * @tparam Visitor function for perform what you want with quadrant
 * @param node_box Main 2D bounding box
 * @param value_box Any 2D bounding box inside the node_box
 * @param visitor function to pass a quafrant in
 */
template <typename Visitor>
  requires std::is_invocable_v<Visitor, pos_declaration::quadrant>
inline void
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
quadrants(box_t node_box, box_t value_box, Visitor visitor) noexcept(
    std::is_nothrow_invocable_v<Visitor, pos_declaration::quadrant>) {
  using namespace pos_declaration;
  auto center = details::center(node_box);
  // S +-------- X
  //   | +---+ |
  //   |-|-+-|-|
  //   | +---+ |
  // Y --------+ E
  if (value_box[S][Y] <= center[Y]) {
    if (value_box[S][X] <= center[X]) {
      visitor(quadrant::top_left);
    }
    if (value_box[E][X] > center[X]) {
      visitor(quadrant::top_right);
    }
  }

  if (value_box[E][Y] > center[Y]) {
    if (value_box[S][X] <= center[X]) {
      visitor(quadrant::bottom_left);
    }
    if (value_box[E][X] > center[X]) {
      visitor(quadrant::bottom_right);
    }
  }
}

/**
 * @brief Compute a child box by given quadrant from the parent box
 *
 * @param parent a parent quad tree rect
 * @param child a quadrant
 * @return a child quad tree rect
 */
inline box_t compute_child_rect(box_t parent,
                                pos_declaration::quadrant child) noexcept {
  using namespace pos_declaration;
  point_t child_size = (parent[E] - parent[S]) / point_t(2);
  switch (child) {
  case quadrant::top_left:
    // S +--------
    //   | v |   |
    //   |---+---|
    //   |   |   |
    //   --------+ E
    return {parent[S], parent[S] + child_size};
  case quadrant::top_right:
    // S +---+----
    //   |   | v |
    //   |-------+
    //   |   |   |
    //   --------+ E
    return {{parent[S].x + child_size.x, parent[S].y},
            {parent[E].x, parent[S].y + child_size.y}};
  case quadrant::bottom_left:
    // S +--------
    //   |   |   |
    //   +-------|
    //   | v |   |
    //   ----+---+ E
    return {{parent[S].x, parent[S].y + child_size.y},
            {parent[S].x + child_size.x, parent[E].y}};
  case quadrant::bottom_right:
    // S +--------
    //   |   |   |
    //   |---+---|
    //   |   | v |
    //   --------+ E
    return {parent[S] + child_size, parent[E]};
  default:
    break;
  }
  assert(false && "Invalid child quadrant");
  return {};
}

} // namespace details

/**
 * @class quad_tree
 * @brief Quad tree collision detection algorithm
 *
 * @tparam ID_Type How to represent an entity in quad_tree
 *
 */
template <typename ID_Type> class quad_tree {
private:
  // is a max number of values a node can contain before we try to split it
  static constexpr uint8_t MAX_ELEMENTS = 8;
  // Stores the maximum depth allowed for the quadtree.
  static constexpr uint8_t MAX_DEPTH = 8;

  // Some imroves for code-readability, -1 is a marker for many things
  // in the quad_tree implementation, used for mark the end of linked lists
  static constexpr int8_t NO_CHILDREN = -1;
  // This node is a branch, this value sets to node::count_entities
  static constexpr int8_t IS_BRANCH = -1;
  static constexpr indx_t INVALID_INDEX = -1;

  // represent an entity in the quadtree.
  // An element is only inserted once to the quadtree no matter how many cells
  // it occupies.
  struct entity {
    box_t bbox;
    ID_Type id;
  };
  // Stores all real actors
  free_list::free_list<entity> entities_;

  struct node {
    // Points to the first child if this node is a branch or the first
    // element if this node is a leaf.
    indx_t first_child = NO_CHILDREN;
    // Stores the number of elements in the node or -1 if it is not a leaf.
    int count_entities = IS_BRANCH;
    [[nodiscard]] bool is_leaf() const noexcept {
      return count_entities != IS_BRANCH;
    }
  };

  // Stores all the nodes in the quad tree. The first node in this sequence is
  // always the root. All another nodes represents branches and leaves
  free_list::free_list<node> nodes_;

  // for each cell it occupies, an "element node" is inserted which indexes
  // that element. singly-linked index list node into an array
  struct entity_node {
    // Points to the next element in the leaf node.
    indx_t next_entity_in_leaf = NO_CHILDREN;
    indx_t indx_in_entities = INVALID_INDEX;
  };
  // Store data for each real entities from the quad_tree::entities_.
  // Each entity may have more than one node if it contains in more than one
  // leaf
  free_list::free_list<entity_node> e_nodes_;

  // Stores temporary data about a leaf or a branch node during a leaves search.
  struct node_search_data {
    box_t rect{};
    indx_t indx_in_nodes = INVALID_INDEX;
    int depth = INVALID_INDEX;
  };

  // Stores the quadtree extents.
  box_t root_rect_;

  // Stores the first free node in the quadtree to be reclaimed as 4
  // contiguous nodes at once. A value of -1(end_of_list) indicates that the
  // free list is empty, at which point we simply insert 4 nodes to the back
  // of the quad_tree::nodes array.
  // indx_t first_free_node_ = NO_CHILDREN;

public:
  // ┌──────────────────────────────────────────────────────────┐
  // │  Interface                                               │
  // └──────────────────────────────────────────────────────────┘

  explicit quad_tree(box_t root_rect)
      : root_rect_(root_rect), // first_free_node_(NO_CHILDREN),
        nodes_{/*a root node*/ node{.first_child = NO_CHILDREN,
                                    .count_entities = 0}} {}

  [[nodiscard]] box_t get_root_rect() const noexcept { return root_rect_; }

  /**
   * @class inserted
   * @brief Index for the inserted element in the quad tree
   *
   */
  class inserted {
    friend class quad_tree<ID_Type>;
    indx_t i_;
    explicit inserted(indx_t i) : i_(i){};
  };

  /**
   * @brief cInsert element into quad tree
   *
   * @param item item id
   * @param bbox item extents
   * @return Index to inserted element
   */
  [[nodiscard]] inserted insert(ID_Type item, box_t item_box);

  /**
   * @brief Remove an element from the quad tree
   *
   * @param indx key for inserted element
   */
  void remove(inserted indx);

  /**
   * @brief Find all intersections with given rect
   *
   * @tparam Visitor Function for call for each collider
   * @param rect Input rectangle
   * @param what_do_with_collider Visitor instance
   */
  template <typename Visitor>
    requires std::is_invocable_v<Visitor, ID_Type &>
  void query(box_t bbox, Visitor what_do_with_collider) {
    // Find the leaves that intersect the specified query rectangle.
    // For each leaf node, look for elements that intersect.
    // binary search leaves which `rect` intersects
    if (details::intersect(bbox, root_rect_)) {
      find_leaves(
          root_search_data(), bbox,
          [this, bbox,
           what_do_with_collider = std::forward<Visitor>(what_do_with_collider),
           traversed = boost::dynamic_bitset<>(entities_.all_size())](
              const node_search_data &leaf) mutable {
            const node &leaf_node = nodes_[leaf.indx_in_nodes];

            // Walk the leaf elements and add elements that intersect.
            auto entity_node_i = leaf_node.first_child;
            while (entity_node_i != NO_CHILDREN) {
              const auto entity_i = e_nodes_[entity_node_i].indx_in_entities;
              if (entities_[entity_i].bbox != bbox && !traversed[entity_i] &&
                  details::intersect(bbox, entities_[entity_i].bbox)) {
                what_do_with_collider(entities_[entity_i].id);
                traversed[entity_i] = true;
              }
              entity_node_i = e_nodes_[entity_node_i].next_entity_in_leaf;
            }
          });
    }
  }

  /**
   * @brief Defferer cleanup, Should call after delete all moved actors
   * for merge empty leaves
   */
  void cleanup();

  /**
   * @brief Get entity by  'inserted' key
   * This function is for debugging purpose
   *
   * @param element Entity key
   * @return entity
   */
  [[nodiscard]] entity inspect_entity(inserted element) const {
    return entities_[element.i_];
  }

private:
  // start point for traverse through all nodes
  [[nodiscard]] node_search_data root_search_data() const {
    return {.rect = root_rect_, .indx_in_nodes = 0, .depth = 0};
  }
  // insert entity into the given leaf
  void insert_into_leaf(const node_search_data &leaf, indx_t element);

  // split a leaf into 4 leaves
  void split(const node_search_data &leaf);
  // merge 4 leaves into one
  void merge(indx_t branch_node_i_in_nodes);

  // main function for perform binary search for leaf or branch that intersect
  // with given start_rect if a start_rect is a root, than all leaves and
  // branches will be visited
  template <typename BranchVisitor, typename LeafVisitor>
    requires std::is_invocable_v<BranchVisitor, const node_search_data &> &&
             std::is_invocable_v<LeafVisitor, const node_search_data &>
  void traverse(const node_search_data &start, box_t start_rect,
                BranchVisitor branch_visitor, LeafVisitor leaf_visitor) const {

    std::stack<node_search_data> to_process;
    to_process.push(start);

    while (!to_process.empty()) {
      node_search_data nd_data = std::move(to_process.top());
      to_process.pop();
      if (nodes_[nd_data.indx_in_nodes].is_leaf()) {
        leaf_visitor(nd_data);
      } else {
        branch_visitor(nd_data);
        // Check if this branch is branch yet after the branch_visitor
        // Cleanup query may delete all empty children
        if (!nodes_[nd_data.indx_in_nodes].is_leaf()) {
          // Push the children that intersect the rectangle. To process
          // in the next iteration
          details::quadrants(
              nd_data.rect, start_rect,
              [this, &to_process,
               &nd_data](details::pos_declaration::quadrant q) {
                to_process.push(node_search_data{
                    .rect = details::compute_child_rect(nd_data.rect, q),
                    .indx_in_nodes = nodes_[nd_data.indx_in_nodes].first_child +
                                     static_cast<indx_t>(q),
                    .depth = nd_data.depth + 1});
              });
        }
      }
    }
  }
  template <typename Visitor>
    requires std::is_invocable_v<Visitor, const node_search_data &>
  void find_leaves(const node_search_data &start, box_t start_rect,
                   Visitor &&what_do_with_leaf) const {
    traverse(
        start, start_rect, [](const node_search_data &) {},
        std::forward<Visitor>(what_do_with_leaf));
  }
};

// ┌──────────────────────────────────────────────────────────┐
// │ Inplementation                                           │
// └──────────────────────────────────────────────────────────┘

template <typename T>
quad_tree<T>::inserted quad_tree<T>::insert(T item, box_t item_box) {
  const indx_t element = entities_.emplace(entity{item_box, std::move(item)});
  // Find the leaves that intersect with entity rect
  // and insert the element to all the leaves found.
  find_leaves(root_search_data(), item_box, // entities_[element].bbox,
              [this, element](const node_search_data &leaf) {
                insert_into_leaf(leaf, element);
              });

  return inserted{element};
}

template <typename T>
void quad_tree<T>::insert_into_leaf(const node_search_data &leaf,
                                    indx_t element) {
  node &leaf_node = nodes_[leaf.indx_in_nodes];

  // Insert the element node to the leaf.
  leaf_node.first_child =
      e_nodes_.emplace(entity_node{.next_entity_in_leaf = leaf_node.first_child,
                                   .indx_in_entities = element});
  // If the leaf is full, split it.
  if (leaf_node.count_entities == MAX_ELEMENTS && leaf.depth < MAX_DEPTH) {
    split(leaf);
  } else {
    // Increment the leaf element count.
    ++leaf_node.count_entities;
  }
}

template <typename T>
void quad_tree<T>::split(const node_search_data &leaf_data) {
  assert(nodes_[leaf_data.indx_in_nodes].is_leaf() &&
         "Only leaves can be split");
  // Pop off all the previous elements.
  auto elts = std::deque<indx_t>{};
  {
    auto &leaf_node = nodes_[leaf_data.indx_in_nodes];
    while (leaf_node.first_child != NO_CHILDREN) {
      const indx_t index =
          std::exchange(leaf_node.first_child,
                        e_nodes_[leaf_node.first_child].next_entity_in_leaf);
      elts.push_back(e_nodes_[index].indx_in_entities);
      e_nodes_.erase(index);
    }
  }
  // Start by allocating 4 child nodes.
  // NOTE: Do not use reference or pointers for leaf_node here
  // NOTE: continuos emplace into free list based on the fact that
  // we always deletes all 4 leaves from the quad_tree::nodes_ free list
  // and than insert 4 leaves, and the order for the each 4 leaves always keeps
  nodes_[leaf_data.indx_in_nodes].count_entities = IS_BRANCH;
  nodes_[leaf_data.indx_in_nodes].first_child =
      // first child
      nodes_.emplace(node{.first_child = NO_CHILDREN, .count_entities = 0});
  for (int i = 0; i < 3; ++i) {
    // other children
    auto j =
        nodes_.emplace(node{.first_child = NO_CHILDREN, .count_entities = 0});
  }

  // Transfer the elements in the former leaf node to its new children.
  for (const auto &elt : elts) {
    find_leaves(leaf_data, entities_[elt].bbox,
                [this, elt](const node_search_data &leaf) {
                  insert_into_leaf(leaf, elt);
                });
  }
}

template <typename T> void quad_tree<T>::remove(inserted indx) {
  // For each leaf node that intersect with the entity rect, remove the entity
  // node from quad_tree::e_nodes_
  find_leaves(root_search_data(), entities_[indx.i_].bbox,
              [indx, this](const node_search_data &leaf) {
                node &node = nodes_[leaf.indx_in_nodes];
                // Walk the list until we find the element node.
                indx_t e_node_i = node.first_child;
                indx_t prev_i = INVALID_INDEX;
                while (e_node_i != NO_CHILDREN &&
                       e_nodes_[e_node_i].indx_in_entities != indx.i_) {
                  prev_i = e_node_i;
                  e_node_i = e_nodes_[e_node_i].next_entity_in_leaf;
                  assert(e_node_i != NO_CHILDREN &&
                         "Inserted entity not found in the leaf");
                }

                if (e_node_i != NO_CHILDREN) {
                  // Remove the element node.
                  if (prev_i == INVALID_INDEX) {
                    // Previous entity not exists, set the next entity as a
                    // first
                    node.first_child = e_nodes_[e_node_i].next_entity_in_leaf;
                  } else {
                    // remove current from the chain between the prev and the
                    // next
                    e_nodes_[prev_i].next_entity_in_leaf =
                        e_nodes_[e_node_i].next_entity_in_leaf;
                  }
                  e_nodes_.erase(e_node_i);
                  --node.count_entities;
                }
              });

  // Remove the element.
  entities_.erase(indx.i_);
}

template <typename T> void quad_tree<T>::cleanup() {
  // // Only process the root if it's not a leaf.
  if (!nodes_[0].is_leaf()) {
    traverse(
        root_search_data(), root_rect_,
        // Branch Visitor
        [&](const node_search_data &branch) {
          // Loop through the children.
          int num_empty_leaves = 0;
          for (int i = 0; i < 4; ++i) {
            // Increment empty leaf count if the child is an empty leaf.
            if (nodes_[i + nodes_[branch.indx_in_nodes].first_child]
                    .count_entities == 0) {
              ++num_empty_leaves;
            }
          }
          // If all the children were empty leaves, remove them and
          // make this node the new empty leaf.
          if (num_empty_leaves == 4) {
            merge(branch.indx_in_nodes);
          }
        },
        // Leaf visitor
        [](const node_search_data &) {});
  }
}

template <typename T> void quad_tree<T>::merge(indx_t branch_node_i_in_nodes) {
  // NOTE: Remove all 4 children in reverse order so that they
  // can be reclaimed on subsequent insertions in proper
  // order.
  for (int i = 3; i >= 0; --i) {
    nodes_.erase(nodes_[branch_node_i_in_nodes].first_child + i);
  }
  // And Make this node the new empty leaf.
  nodes_[branch_node_i_in_nodes].first_child = NO_CHILDREN;
  nodes_[branch_node_i_in_nodes].count_entities = 0;
}

} // namespace collision

#endif
