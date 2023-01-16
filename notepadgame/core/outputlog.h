
#pragma once

#include <string>
#include <fstream>




// TODO add concept wstreamble

class outputlog final
{
public:
    
    inline static std::wstring main_log_file{L"W:\\raw-c++_projects\\notepadgame\\MyLogFile.txt"};
    
    static outputlog& glog()
    {
        static outputlog glog{main_log_file};
        return glog;
    }

    outputlog() = delete;
    
    explicit outputlog(const std::wstring& file)
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

    explicit outputlog(std::wofstream stream) : stream_(std::move(stream)){}
    
    outputlog(const outputlog& other) = delete;
 
    outputlog(outputlog&& other) noexcept // move constructor
    : stream_(std::move(other.stream_)) {}
 
    outputlog& operator=(const outputlog& other) = delete;
    
    outputlog& operator=(outputlog&& other) noexcept // move assignment
    {
        std::swap(stream_, other.stream_);
        return *this;
    }
    
    virtual ~outputlog()
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
    std::wofstream stream_;
};
