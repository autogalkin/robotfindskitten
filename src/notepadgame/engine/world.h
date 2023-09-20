#pragma once
#include "details/nonconstructors.h"
#include <stdint.h>
#include <type_traits>
#pragma warning(push, 0)
#include "df/dirtyflag.h"
#include <memory>
#include <vector>
#pragma warning(pop)
#include "details/base_types.h"
#include "ecs_processor_base.h"
#include "tick.h"

class engine_t;

// the array of chars on the screen
class backbuffer {
public:
  explicit backbuffer(engine_t *owner);
  virtual ~backbuffer() = default;

  void init(const npi_t chars_in_line, const npi_t lines_on_screen);

  // paste chars to the edit contol
  void send();

  [[nodiscard]] virtual char_size &at(const position_t &char_on_screen);
  [[nodiscard]] virtual char_size at(const position_t &char_on_screen) const;
  [[nodiscard]] position_t
  global_position_to_buffer_position(const position_t &position) const noexcept;
  [[nodiscard]] bool
  is_in_buffer(const position_t &global_position) const noexcept;
  void draw(const position_t &pivot, const shape::sprite &sh, int32_t depth);
  void erase(const position_t &pivot, const shape::sprite &sh, int32_t depth);

private:
  // visitor traverse all sprite matrix
  void traverse_sprite_positions(
      const position_t &pivot, const shape::sprite &sh,
      const std::function<void(const position_t &, char_size part_of_sprite)>
          &visitor) const;


  int32_t width_;
  int32_t height_;
  std::vector<int32_t> z_buffer_;
  std::vector<char_size> buffer_;



  // TODO flat line buffer
  //using line_type = df::dirtyflag<std::vector<char_size>>;
  //std::unique_ptr<std::vector<line_type>> buffer{};
  engine_t *engine_;
  df::dirtyflag<position_t> scroll_{{}, df::state::dirty};
  boost::signals2::scoped_connection scroll_changed_connection_;
  boost::signals2::scoped_connection size_changed_connection;
};

class ecs_processors_executor {
public:
  enum class insert_order : int8_t { before = 0, after = 1 };
  bool insert_processor_at(
      std::unique_ptr<ecs_processor> who,
      const std::type_info& near_with, /* typeid(ecs_processor) */
      const insert_order where = insert_order::before);
  void execute(entt::registry &reg, const gametime::duration delta) const {
    for (const auto &i : data_) {
      i->execute(reg, delta);
    }
  }

  template <typename T, typename... Args>
    requires std::derived_from<T, ecs_processor> &&
             std::is_constructible_v<T, Args...>
  void push(Args &&...args) {
    data_.emplace_back(std::make_unique<T>(std::forward<Args>(args)...));
  }

private:
  std::vector<std::unique_ptr<ecs_processor>> data_;
};



class world final:
      public tickable, nonmoveable, noncopyable
{
public:
  explicit world(engine_t *owner) noexcept;
  ~world() override;
    
  ecs_processors_executor executor;
  backbuffer backbuffer;
  entt::registry reg_;
  void tick(gametime::duration delta) override {
    executor.execute(reg_, delta);
  }

    template<typename F>
    requires std::is_invocable_v<F, entt::registry &, const entt::entity>
    void spawn_actor(F&& for_add_components){
        const auto entity = reg_.create();
        for_add_components(reg_, entity);
    }
private:
  void redraw_all_actors();
  engine_t *engine_;
  boost::signals2::scoped_connection scroll_changed_connection_;
  boost::signals2::scoped_connection size_changed_connection;
};

