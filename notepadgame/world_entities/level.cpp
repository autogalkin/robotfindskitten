#include "level.h"

#include <boost/range/adaptor/indexed.hpp>
#include "../world.h"

backbuffer::backbuffer(backbuffer&& other) noexcept: line_lenght_(other.line_lenght_),
                                                     lines_count_(other.lines_count_),
                                                     buffer(std::move(other.buffer)),
                                                     world_(other.world_),
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
    world_ = other.world_;
    scroll = std::move(other.scroll);
    return *this;
}

char backbuffer::at(translation char_on_screen) const
{
    return (*buffer)[char_on_screen.line()].pin()[char_on_screen.index_in_line()];
}

char& backbuffer::at(translation char_on_screen)
{
    (*buffer)[char_on_screen.line()].mark_dirty();
    return (*buffer)[char_on_screen.line()].pin()[char_on_screen.index_in_line()];
}

void backbuffer::init(const int window_width)
{
    const int char_width = world_->get_char_width();
    line_lenght_ = window_width / char_width;
    lines_count_ = world_->get_lines_on_screen();
    
    if(!buffer) buffer = std::make_unique<std::vector< dirty_flag< std::vector<char> > > >(lines_count_);
    else buffer->resize(lines_count_);
    for(auto& line  : *buffer)
    {
        line.pin().resize(line_lenght_+2, ' '); // 2 for \n and \0
        line.pin().back() = '\0';
    }

}

void backbuffer::send()
{
    const auto pos = world_->get_caret_index();
    const int64_t fl = world_->get_first_visible_line();
    const int64_t h_scroll = world_->get_horizontal_scroll_offset() / world_->get_char_width();
    
    bool change_buffer = false;
    for(const auto [ i, linedata] : *buffer | boost::adaptors::indexed(0))
    {
        if(auto& line = linedata; line.is_changed() || scroll.is_changed())
        {
            const int64_t lnum = fl + i;
            const int64_t fi = world_->get_first_char_index_in_line(lnum);
            const int64_t ll = world_->get_line_lenght(lnum); // include endl if exists
            int64_t endsel = h_scroll + line_lenght_+endl > ll-endl ? ll : line_lenght_ + h_scroll+1;
           
            char ch_end {'\0'};
            if(lnum >= world_->get_lines_count()-1)    { ch_end = '\n';   }
            else                                       { endsel  -= endl; }
            
            *(line.pin().end() - 1/*past-the-end*/ - 1 /* '\0' */) = ch_end; 
            world_->set_selection(fi+h_scroll,  fi  + endsel);
            
            world_->replace_selection({line.get().begin(), line.get().end()});
            line.reset_flag();
            change_buffer = true;
            
        }
    }
    
    if(change_buffer)
    {
        world_->set_caret_index(pos);
        scroll.reset_flag();
    } 
}

void backbuffer::get() const
{
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
}


