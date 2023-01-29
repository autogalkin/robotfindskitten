
#pragma once


#include <any>
#include <iostream>



template<typename T>
concept is_streamble = requires(T t, std::ostream& stream)
{
    { stream << t } ->  std::convertible_to<std::ostream &>;
};

class gamelog final
{
public:
    
    static gamelog& get()
    {
        static gamelog glog{};
        return glog;
    }

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
            std::cout  << args << sep << std::endl;

        } (), ...);
    }

protected:
    explicit gamelog()
    {
        AllocConsole();
        FILE* fdummy;
        auto err = freopen_s(&fdummy, "CONOUT$", "w", stdout);
        err = freopen_s(&fdummy, "CONOUT$", "w", stderr);
        std::cout.clear();
        std::clog.clear();
        const HANDLE hConOut = CreateFile(L"CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        SetStdHandle(STD_OUTPUT_HANDLE, hConOut);
        SetStdHandle(STD_ERROR_HANDLE, hConOut);
        std::wcout.clear();
        std::wclog.clear();
    }
    
    virtual ~gamelog()
    {
        FreeConsole();
    }
    
};
