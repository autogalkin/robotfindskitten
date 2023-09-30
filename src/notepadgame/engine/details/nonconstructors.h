#pragma once

// NOLINTBEGIN(cppcoreguidelines-special-member-functions)
class noncopyable {
public:
    noncopyable(const noncopyable&) = delete;
    noncopyable& operator=(const noncopyable&) = delete;

protected:
    noncopyable() = default;
    ~noncopyable() = default;
};

class nonmoveable {
public:
    nonmoveable(nonmoveable&&) = delete;
    nonmoveable& operator=(nonmoveable&&) = delete;

protected:
    nonmoveable() = default;
    ~nonmoveable() = default;
};
// NOLINTEND(cppcoreguidelines-special-member-functions)
