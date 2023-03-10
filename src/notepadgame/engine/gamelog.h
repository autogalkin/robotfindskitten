
#pragma once

#include <iostream>

class gamelog final
{
public:
    
    static gamelog& get();
    inline static constexpr char sep = ' ';
    gamelog(const gamelog& other)                = delete;
    gamelog(gamelog&& other) noexcept            = delete;
    gamelog& operator=(const gamelog& other)     = delete;
    gamelog& operator=(gamelog&& other) noexcept = delete;

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
