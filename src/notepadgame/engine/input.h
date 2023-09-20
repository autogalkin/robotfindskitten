#pragma once

#pragma warning(push, 0)
#include "Windows.h"
#include "boost/signals2.hpp"
#include <entt/entt.hpp>
#include <queue>
#pragma warning(pop)

#include "details/base_types.h"
#include "ecs_processor_base.h"
#include "tick.h"

class input final : public tickable, public noncopyable, public nonmoveable {
public:
  // https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
  enum class key : WPARAM {
    w = 0x57,
    a = 0x41,
    s = 0x53,
    d = 0x44,
    l = 0x49,
    space = VK_SPACE,
    up = VK_UP,
    right = VK_RIGHT,
    left = VK_LEFT,
    down = VK_DOWN

  };

  using key_state_type = std::vector<input::key>;
  using signal_type =
      boost::signals2::signal<void(const key_state_type &key_state)>;

  explicit input() = default;
  ~input() override;

  virtual void tick(gametime::duration) override;
  void send_input();
  void receive(const LPMSG msg);
  [[nodiscard]] const key_state_type &get_down_key_state() const {
    return down_key_state_;
  }
  [[nodiscard]] input::signal_type &get_down_signal() { return on_down; }
  void clear_input() { down_key_state_.clear(); }

private:
  signal_type on_down;
  signal_type on_up;
  key_state_type down_key_state_;
  key_state_type up_key_state_;
};
