
#pragma once



#include <iostream>



template<typename T>
concept is_streamble = requires(T t, std::ostream& stream)
{
    { stream << t } ->  std::convertible_to<std::ostream &>;
};

class gamelog final
{
public:
    
    static gamelog& get();
    inline static std::string sep = " ";
    gamelog(const gamelog& other) = delete;
    gamelog(gamelog&& other) noexcept = delete;
    gamelog& operator=(const gamelog& other) = delete;
    gamelog& operator=(gamelog&& other) noexcept = delete;

    template<is_streamble ... Args>
    static void cout( Args&& ... args)
    {
        int i = 0;
        ([&]
        {
            ++i;
            std::cout  << args << sep;

        } (), ...);
        std::cout << std::endl;
    }

protected:
    explicit gamelog();
    virtual ~gamelog();
};
