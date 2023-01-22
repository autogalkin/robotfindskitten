
#pragma once

#include <string>
#include <fstream>




// TODO add concept wstreamble

class outlog final
{
public:
    
    inline static std::wstring main_log_file{L"W:\\raw-c++_projects\\notepadgame\\mainlog.txt"};
    
    static outlog& get()
    {
        static outlog glog{main_log_file};
        return glog;
    }

    outlog() = delete;
    
    explicit outlog(const std::wstring& file)
    {
        stream_.exceptions(std::wofstream::failbit | std::wofstream::badbit);
        try
        {
            stream_.open(file, std::ios::app); // open the existing file
            
        } catch (std::wofstream::failure&) {
            
            stream_.clear();
            stream_.open(file, std::ios::out); // create a new file
            if(!stream_.is_open()) throw;
        }
    }

    explicit outlog(std::ofstream stream) : stream_(std::move(stream)){}
    
    outlog(const outlog& other) = delete;
 
    outlog(outlog&& other) noexcept // move constructor
    : stream_(std::move(other.stream_)) {}
 
    outlog& operator=(const outlog& other) = delete;
    
    outlog& operator=(outlog&& other) noexcept // move assignment
    {
        std::swap(stream_, other.stream_);
        return *this;
    }
    
    virtual ~outlog()
    {
        stream_.close();
    }

    
    template<typename ... Args>
    //requires Streamable
    void print(Args... args)
    {
        (stream_ <<  ... << args)  <<std::endl;
    }

private:
    std::ofstream stream_;
};
