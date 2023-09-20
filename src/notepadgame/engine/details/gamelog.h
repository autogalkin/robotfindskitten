
#pragma once

#include "details/nonconstructors.h"
#include <iostream>

class gamelog final : public noncopyable, public nonmoveable {
public:
  static gamelog &get();
  inline static constexpr char sep = ' ';

  template <typename... Args>
    requires requires(Args... args, std::ostream &stream) {
      { (stream << ... << args) } -> std::convertible_to<std::ostream &>;
    }
  static void cout(Args &&...args) {
#ifndef NDEBUG
    ([&] { std::cout << args << sep; }(), ...);
    std::cout << std::endl;
#endif // DEBUG
  }

protected:
  explicit gamelog();
  ~gamelog();
};
