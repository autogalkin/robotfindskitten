#include "world.h"


#include "notepader.h"
#include "../core/engine.h"
#include "../../extern/range-v3-0.12.0/include/range/v3/view/enumerate.hpp"
#include "../../extern/range-v3-0.12.0/include/range/v3/view/filter.hpp"
#include "utf8.h"
backbuffer::backbuffer(backbuffer&& other) noexcept: line_lenght_(other.line_lenght_),
                                                     lines_count_(other.lines_count_),
                                                     buffer(std::move(other.buffer)),
                                                     engine_(other.engine_),
                                                     scroll(std::move(other.scroll))
{
}

backbuffer& backbuffer::operator=(backbuffer&& other) noexcept
{
    if (this == &other)
        return *this;
    line_lenght_ = other.line_lenght_;
    lines_count_ = other.lines_count_;
    buffer = std::move(other.buffer);
    engine_ = other.engine_;
    scroll = std::move(other.scroll);
    return *this;
}



void backbuffer::draw(const position& pivot, const shape::sprite& sh)
{
    for(auto rows = sh.data.rowwise();
        auto [line, row] : rows | ranges::views::enumerate)
    {
        
        auto& [line_chars, _] = (*buffer)[line + pivot.line()].pin();
        
        for(int ind_in_line{-1}; const char_size ch : row
            | ranges::views::filter([&ind_in_line](const char c){++ind_in_line; return c != shape::whitespace;}))
        {
                position p = pivot + position{static_cast<npi_t>(line), ind_in_line};
                at(p) = ch;
        }
    }
}

void backbuffer::erase(const position& pivot, const shape::sprite& sh)
{
    for(auto rows = sh.data.rowwise();
        auto [line, row] : rows | ranges::views::enumerate)
    {
        
        auto& [line_chars,_] = (*buffer)[line + pivot.line()].pin();
        
        for(int byte_i{-1}; const auto byte : row
            | ranges::views::filter([&byte_i](const char_size c){++byte_i; return c != shape::whitespace;}))
        {
                position p = pivot + position{static_cast<npi_t>(line), byte_i};
                at(p) = shape::whitespace;
        }
    }
}

char_size backbuffer::at(const position& char_on_screen) const
{
    auto& [chars, _] = (*buffer)[char_on_screen.line()].get();
    return chars[char_on_screen.index_in_line()];
}
char_size& backbuffer::at(const position& char_on_screen)
{
    auto& [chars, _] = (*buffer)[char_on_screen.line()].pin();
    return chars[char_on_screen.index_in_line()];
}

backbuffer::~backbuffer() = default;

void backbuffer::init(const int window_width)
{
    const int char_width = engine_->get_char_width();
    line_lenght_ = window_width / char_width;
    lines_count_ = engine_->get_lines_on_screen();
    
    if(!buffer) buffer = std::make_unique<std::vector< line_type > >(lines_count_);
    else buffer->resize(lines_count_);
    for(auto& line  : *buffer)
    {
        line.pin().chars.resize(line_lenght_+2, ' '); // 2 for \n and \0
        *(line.pin().chars.end() - 1/*past-the-end*/ - 1 /* '\0' */) = '\n';
        line.pin().chars.back() = '\0';
    }
    
}

void backbuffer::send()
{
    const auto pos = engine_->get_caret_index();
    bool buffer_is_changed = false;
   
    
    for(int enumerate{-1}; auto& line : *buffer // | ranges::views::enumerate
        | ranges::views::filter([this, &enumerate](auto& l){++enumerate; return l.is_changed() || scroll.is_changed() ;}))
    {

        
        
        
        const npi_t lnum = scroll.get().line() + enumerate;
        const npi_t fi = engine_->get_first_char_index_in_line(lnum);
        //const npi_t ll = engine_->get_line_lenght(lnum); // include endl if exists
        //npi_t endsel = scroll.get().index_in_line() + line.get().chars.size() - 2 + endl > ll - endl ? ll : line_lenght_ + scroll.get().index_in_line() + 1;
        npi_t endsel  = std::max(0, line.get().pasted_bytes_count-1);
        char_size ch_end {'\0'};
        if(lnum >= engine_->get_lines_count()-1)    { ch_end = '\n';  }
        else                                       { endsel  -= endl; }
    
        *(line.pin().chars.end() - 1/*past-the-end*/ - 1 /* '\0' */) = ch_end;


        std::vector<char> bytes{};
        utf8::utf32to8(line.get().chars.begin(), line.get().chars.end(), std::back_inserter(bytes));
        
        engine_->set_selection(fi+scroll.get().index_in_line(), fi + endsel);
        
        engine_->replace_selection({bytes.begin(), bytes.end()});
        line.pin().pasted_bytes_count = static_cast<int>(bytes.size());
        line.reset_flag();
        buffer_is_changed = true; 
       
        
    }
    
    if(buffer_is_changed)
    {
        engine_->set_caret_index(pos);
        scroll.reset_flag();
    } 
}

void backbuffer::get() const
{
    /*
    const auto pos = world_->get_caret_index();
    const int64_t fl = world_->get_first_visible_line();
    const int64_t h_scroll = world_->get_horizontal_scroll_offset() / world_->get_char_width();
    
    for(const auto [i, linedata] : *buffer | boost::adaptors::indexed(0))
    {
        if(auto&  line = linedata; line.is_changed())
        {
            const int64_t lnum = fl + i;
            const int64_t fi = world_->get_first_char_index_in_line(lnum);
            
            world_->set_selection(fi+h_scroll,  fi  + line_lenght_ + h_scroll);
            world_->get_selection_text(line.pin());
            line.reset_flag();
        }
    }
    world_->set_caret_index(pos);
    */
}



world::~world() = default;

