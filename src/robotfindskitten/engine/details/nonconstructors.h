#pragma once

class noncopyable {
public:
    noncopyable(noncopyable&&) = default;
    noncopyable& operator=(noncopyable&&) = default;

    noncopyable(const noncopyable&) = delete;
    noncopyable& operator=(const noncopyable&) = delete;

protected:
    noncopyable() = default;
    ~noncopyable() = default;
};

class nonmoveable {
public:
    nonmoveable(const nonmoveable&) = default;
    nonmoveable& operator=(const nonmoveable&) = default;

    nonmoveable(nonmoveable&&) = delete;
    nonmoveable& operator=(nonmoveable&&) = delete;

protected:
    nonmoveable() = default;
    ~nonmoveable() = default;
};
