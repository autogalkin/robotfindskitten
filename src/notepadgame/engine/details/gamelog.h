﻿
#pragma once

#include <iostream>
#include "details/nonconstructors.h"


class gamelog final : public noncopyable, public nonmoveable
{
public:
    
    static gamelog& get();
    inline static constexpr char sep = ' ';

    template<typename ... Args>
    requires requires(Args... args, std::ostream& stream){
        {  (stream << ... << args) } ->  std::convertible_to<std::ostream &>;
    }
    static void cout( Args&& ... args)
    {
        ([&]
        {
            std::cout  << args << sep;
        } (), ...);
        std::cout << std::endl;
    }

protected:
    explicit gamelog();
    ~gamelog();
};
