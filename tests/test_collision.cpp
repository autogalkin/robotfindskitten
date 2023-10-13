#include <algorithm>
#include <optional>
#include <random>
#include <sstream>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "game/ecs_processors/collision.h"

// NOLINTBEGIN(readability-magic-numbers)

TEST(FreeListTest, GetAllSize) {
  using namespace collision;
  auto list = free_list::free_list<int>();
  EXPECT_EQ(list.all_size(), 0);
  for (int i = 1; i < 128 * 3; i++) {
    static_cast<void>(list.emplace(i));
    EXPECT_EQ(list.all_size(), i);
  }
}

TEST(FreeListTest, InsertOneItem) {
  using namespace collision;
  auto list = free_list::free_list<int>();
  auto index = list.emplace(5);
  EXPECT_EQ(list.all_size(), 1);
  EXPECT_EQ(index, 0);
  EXPECT_EQ(list[index], 5);
}

TEST(FreeListTest, InsertManyItems) {
  using namespace collision;
  auto list = free_list::free_list<int>();
  for (int i = 0; i < 128 * 2; i++) {
    auto index = list.emplace(i);
    EXPECT_EQ(index, i);
  }
  for (int i = 0; i < 128 * 2; i++) {
    EXPECT_EQ(list[i], i);
  }
}

TEST(FreeListTest, EraseItem) {
  using namespace collision;
  auto list = free_list::free_list<int>();
  auto index = list.emplace(5);
  EXPECT_EQ(index, 0);
  list.erase(index);
  EXPECT_EQ(list.all_size(), 1);
  EXPECT_THROW(static_cast<void>(list[index]), free_list::bad_free_list_access);
}

TEST(FreeListTest, EraseAndInsertItemIntoFreeIndex) {
  using namespace collision;
  int list_size = 30;
  auto list = free_list::free_list<int>();
  for (int i = 0; i < list_size; i++) {
    static_cast<void>(list.emplace(i));
  }
  EXPECT_EQ(list.all_size(), list_size);
  std::random_device rd{};
  std::mt19937 gen(rd());
  int erased_size = 10;
  auto data = std::vector<int>(erased_size, 0);
  std::generate(data.begin(), data.end(), [n = 0]() mutable { return n++; });
  std::shuffle(data.begin(), data.end(), gen);

  for (auto i : data) {
    list.erase(i);
  }
  EXPECT_EQ(list.all_size(), list_size);
  for (int i = 0; i < erased_size; i++) {
    static_cast<void>(list.emplace(i));
  }
  EXPECT_EQ(list.all_size(), list_size);
}

TEST(FreeListTest, ClearList) {
  using namespace collision;
  int list_size = 30;
  auto list = free_list::free_list<int>();
  for (int i = 0; i < list_size; i++) {
    static_cast<void>(list.emplace(i));
  }
  EXPECT_EQ(list.all_size(), list_size);
  list.clear();
  EXPECT_EQ(list.all_size(), 0);
  [[maybe_unused]] auto _ = list.emplace(5);
  EXPECT_EQ(list.all_size(), 1);
}

enum class actor : collision::indx_t {};
std::underlying_type<actor>::type as_integral(actor me) {
  return static_cast<std::underlying_type<actor>::type>(me);
}

std::ostream &operator<<(std::ostream &out, actor me) {
  return out << as_integral(me);
}
// Avoid ADL loookup to glm functions for print box values in gtests
struct prect {
  collision::box_t value;
  bool operator==(const prect &rhs) const { return value == rhs.value; }
  bool operator!=(const prect &rhs) const { return value != rhs.value; }
};
std::ostream &operator<<(std::ostream &out, const prect &box) {
  out << "{" << '(' << box.value[0][0] << "," << box.value[0][1] << "),("
      << box.value[1][0] << ',' << box.value[1][1] << ')' << "}";

  return out;
}

TEST(QuadTreeTest, Quadrants) {
  using namespace collision;
  auto rect = box_t{{0, 0}, {10, 10}};
  auto b = box_t{{3, 3}, {8, 8}};
  int call_count = 0;
  using namespace details::pos_declaration;
  details::quadrants(rect, b, [&call_count](quadrant quad) {
    ++call_count;
    EXPECT_THAT(quad, ::testing::AnyOf(quadrant::top_left, quadrant::top_right,
                                       quadrant::bottom_left,
                                       quadrant::bottom_right));
  });
  EXPECT_EQ(call_count, 4);
}
TEST(QuadTreeTest, ComputeChildBox) {
  using namespace collision;
  using namespace details::pos_declaration;
  auto b = box_t{{0, 0}, {10, 10}};
  auto expect = box_t{{0, 0}, {5, 5}};
  auto v = details::compute_child_rect(b, quadrant::top_left);
  EXPECT_EQ(prect{expect}, prect{v});
  expect = box_t{{5, 0}, {10, 5}};
  v = details::compute_child_rect(b, quadrant::top_right);
  EXPECT_EQ(prect{expect}, prect{v});
  expect = box_t{{0, 5}, {5, 10}};
  v = details::compute_child_rect(b, quadrant::bottom_left);
  EXPECT_EQ(prect{expect}, prect{v});
  expect = box_t{{5, 5}, {10, 10}};
  v = details::compute_child_rect(b, quadrant::bottom_right);
  EXPECT_EQ(prect{expect}, prect{v});
}

TEST(QuadTreeTest, InsertOneItem) {
  using namespace collision;
  enum class actor {};
  auto tree = quad_tree<actor>(box_t{{0, 0}, {10, 10}});
  auto a = actor{0};
  auto abox = box_t{{2, 2}, {7, 7}};
  auto index = tree.insert(a, abox);
  EXPECT_EQ(tree.inspect_entity(index).id, a);
  EXPECT_EQ(tree.inspect_entity(index).bbox, abox);
}

TEST(QuadTreeTest, RemoveItem) {
  using namespace collision;
  auto tree = quad_tree<actor>(box_t{{0, 0}, {10, 10}});
  auto index = tree.insert(actor{0}, box_t{{2, 2}, {5, 5}});
  tree.remove(index);
  EXPECT_THROW(static_cast<void>(tree.inspect_entity(index)),
               std::invalid_argument);
}

TEST(QuadTreeTest, SimpleIntersectItems) {
  using namespace collision;
  auto tree = quad_tree<actor>(box_t{{0, 0}, {10, 10}});
  ;
  auto a_box = box_t{{2, 2}, {5, 5}};
  auto a = actor{0};
  auto b = actor{1};
  auto c = actor{2};
  auto d = actor{3};
  static_cast<void>(tree.insert(a, a_box));
  auto b_box = box_t{{0, 0}, {3, 3}};
  ASSERT_TRUE(details::intersect(a_box, b_box));
  static_cast<void>(tree.insert(actor{1}, b_box));
  auto c_box = box_t{{4, 4}, {6, 6}};
  ASSERT_TRUE(details::intersect(a_box, c_box));
  static_cast<void>(tree.insert(c, c_box));
  auto d_box = box_t{{9, 9}, {10, 10}};
  static_cast<void>(tree.insert(c, d_box));
  ASSERT_FALSE(details::intersect(a_box, d_box));

  int count_of_collides = 0;
  tree.query({a, a_box}, [b, c, d, &count_of_collides](actor collider) {
    EXPECT_THAT(collider, ::testing::AnyOf(b, c));
    count_of_collides++;
    EXPECT_NE(collider, d);
  });
  EXPECT_EQ(count_of_collides, 2);
}

class QuadTreeQueryTest : public ::testing::TestWithParam<collision::indx_t> {

};

static constexpr int AREA_SIZE{100};

std::vector<collision::box_t> generate_actors(collision::indx_t n) {
  using namespace collision;
  auto gen = std::default_random_engine();
  auto dist = std::uniform_int_distribution<indx_t>(0, AREA_SIZE);
  auto nodes = std::vector<box_t>();
  nodes.reserve(n);
  for (collision::indx_t i = 0; i < n; ++i) {
    auto p = std::to_array({dist(gen), dist(gen), dist(gen), dist(gen)});
    nodes.push_back({{std::min(p[0], p[1]), std::min(p[2], p[3])},
                     {std::max(p[0], p[1]), std::max(p[2], p[3])}});
  }
  return nodes;
}

template <typename Visitor>
void brute_force_query(collision::box_t rect,
                       const std::vector<collision::box_t> &all,
                       Visitor &&visitor) {
  using namespace collision;
  for (indx_t i = 0; i < static_cast<indx_t>(all.size()); ++i) {
    if (all[i] != rect && details::intersect(all[i], rect)) {
      visitor(actor{i});
    }
  }
}
std::vector<std::pair<actor, prect>>
difference(std::vector<std::pair<actor, prect>> const &v1,
           std::vector<std::pair<actor, prect>> const &v2) {
  std::vector<std::pair<actor, prect>> diff;
  std::set_symmetric_difference(
      v1.begin(), v1.end(), v2.begin(), v2.end(), std::back_inserter(diff),
      [](const auto &a, const auto &b) { return a.first < b.first; });
  return diff;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
void query_test_against_brute_force(
    const std::vector<collision::box_t> &all_actors,
    collision::quad_tree<actor> &tree,
    std::optional<std::vector<bool>> removed_actors = std::nullopt) {

  using namespace collision;
  // Quadtree
  auto quad_tree_intrs = std::vector<std::vector<std::pair<actor, prect>>>();
  {
    quad_tree_intrs.reserve(all_actors.size());
    auto traversed = std::vector<bool>(all_actors.size(), false);
    for (size_t i = 0; i < all_actors.size(); i++) {
      quad_tree_intrs.emplace_back();

      if (!traversed[i] && (!removed_actors || !(*removed_actors)[i])) {
        tree.query({actor{static_cast<indx_t>(i)}, all_actors[i]}, [i, &quad_tree_intrs, &traversed,
                                   &removed_actors,
                                   &all_actors](auto collider) {
          if (!removed_actors ||
              !(*removed_actors)[static_cast<size_t>(collider)]) {
            quad_tree_intrs[i].push_back(
                {collider, prect{all_actors[static_cast<size_t>(collider)]}});
          }
          traversed[static_cast<size_t>(collider)] = true;
        });
      }
    }
  }
  // Brute force
  auto brute_force_intrs = std::vector<std::vector<std::pair<actor, prect>>>();
  {
    brute_force_intrs.reserve(all_actors.size());
    auto traversed = std::vector<bool>(all_actors.size(), false);
    for (size_t i = 0; i < all_actors.size(); i++) {
      brute_force_intrs.emplace_back();
      if (!traversed[i] && (!removed_actors || !(*removed_actors)[i])) {
        brute_force_query(
            all_actors[i], all_actors,
            [i, &brute_force_intrs, &traversed, &removed_actors,
             &all_actors](const actor &collider) {
              if (!removed_actors ||
                  !(*removed_actors)[static_cast<size_t>(collider)]) {
                brute_force_intrs[i].emplace_back(collider,
                     prect{all_actors[static_cast<size_t>(collider)]});
              }
              traversed[static_cast<size_t>(collider)] = true;
            });
      }
    }
  }
  for (size_t i = 0; i < all_actors.size(); i++) {
    std::sort(std::begin(quad_tree_intrs[i]), std::end(quad_tree_intrs[i]),
              [](const auto &a, const auto &b) { return a.first < b.first; });
    std::sort(std::begin(brute_force_intrs[i]), std::end(brute_force_intrs[i]),
              [](const auto &a, const auto &b) { return a.first < b.first; });

    ASSERT_EQ(quad_tree_intrs[i], brute_force_intrs[i])
        << "Actor: " << i << " , Box: " << prect{all_actors[i]}
        << " difference: " <<
        [&]() {
          std::ostringstream s;
          for (auto &i : difference(quad_tree_intrs[i], brute_force_intrs[i])) {
            s << '(' << i.first << ',' << i.second << ')';
          }
          s << '\n';
          return s.str();
        }();
  }
}

TEST_P(QuadTreeQueryTest, RemoveItemsAndCleanUp) {
  using namespace collision;
  auto actors_n = GetParam();
  auto all_actors = generate_actors(actors_n);
  auto tree = quad_tree<actor>(box_t{{0, 0}, {10, 10}});
  auto ins = std::vector<quad_tree<actor>::inserted>{};
  ins.reserve(all_actors.size());
  for (indx_t i = 0; i < static_cast<indx_t>(all_actors.size()); ++i) {
    ins.push_back(tree.insert(actor{i}, all_actors[i]));
  }
  for (size_t i = 0; i < all_actors.size(); ++i) {
    tree.remove(ins[i]);
  }
  tree.cleanup();
  int colliders_count = 0;
  tree.query({actor{-1}, box_t{{0, 0}, {AREA_SIZE, AREA_SIZE}}},
             [&colliders_count](auto  /*collider*/) { ++colliders_count; });;
  EXPECT_EQ(colliders_count, 0);
}

TEST_P(QuadTreeQueryTest, InsertAndQueryItems) {
  auto actors_n = GetParam();
  auto all_actors = generate_actors(actors_n);
  using namespace collision;
  auto tree = quad_tree<actor>(box_t{{0, 0}, {AREA_SIZE, AREA_SIZE}});
  for (indx_t i = 0; i < static_cast<indx_t>(all_actors.size()); i++) {
    static_cast<void>(tree.insert(actor{i}, all_actors[i]));
  }
  query_test_against_brute_force(all_actors, tree);
}

TEST_P(QuadTreeQueryTest, InsertRemoveAndQueryItems) {
  auto actors_n = GetParam();
  auto all_actors = generate_actors(actors_n);
  using namespace collision;
  auto tree = quad_tree<actor>(box_t{{0, 0}, {AREA_SIZE, AREA_SIZE}});
  auto ins = std::vector<quad_tree<actor>::inserted>{};
  ins.reserve(all_actors.size());
  for (indx_t i = 0; i < static_cast<indx_t>(all_actors.size()); i++) {
    ins.push_back(tree.insert(actor{i}, all_actors[i]));
  }
  auto gen = std::default_random_engine();
  auto death = std::uniform_int_distribution(0, 1);
  // Randomly remove some nodes
  auto removed = std::vector<bool>(ins.size());
  std::generate(std::begin(removed), std::end(removed),
                [&gen, &death]() { return death(gen); });

  for (size_t i = 0; i < removed.size(); ++i) {
    if (removed[i]) {
      tree.remove(ins[i]);
    }
  }
  tree.cleanup();
  query_test_against_brute_force(all_actors, tree, std::move(removed));
}


INSTANTIATE_TEST_SUITE_P(Power10, QuadTreeQueryTest,
                         ::testing::Values(10, 100, 1000, 10000));

// NOLINTEND(readability-magic-numbers)
