#pragma once

class noncopyable                         //NOLINT cppcoreguidelines-special-member-functions
{
public:
  noncopyable(const noncopyable &)            = delete;
  noncopyable &operator=(const noncopyable &) = delete;
protected:
    noncopyable() = default;
   ~noncopyable() = default;
};

class nonmoveable                         //NOLINT cppcoreguidelines-special-member-functions
{
public:
    nonmoveable(nonmoveable &&)            = delete;
    nonmoveable &operator=(nonmoveable &&) = delete;
protected:
    nonmoveable() = default;
   ~nonmoveable() = default;
};
