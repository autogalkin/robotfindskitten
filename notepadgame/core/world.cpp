#include "world.h"


#include "notepader.h"
#include "../core/engine.h"
#include "../../extern/range-v3-0.12.0/include/range/v3/view/enumerate.hpp"
#include "../../extern/range-v3-0.12.0/include/range/v3/view/filter.hpp"
#include "utf8.h"


backbuffer::backbuffer(engine* owner): engine_(owner){
    scroll_changed_connection_ = engine_->get_on_scroll_changed().connect([this](const position& new_scroll)
    {
        scroll_.pin() = new_scroll;
        backbuffer::init();
    });

    size_changed_connection = engine_->get_on_resize().connect([this](uint32_t width, uint32_t height){
        backbuffer::init();
    });
}

position backbuffer::global_position_to_buffer_position(const position& position) const noexcept{
    return {position.line() - scroll_.get().line(), position.index_in_line() - scroll_.get().index_in_line()};
}

bool backbuffer::is_in_buffer(const position& position) const noexcept{
    return position.line() >= scroll_.get().line()
    && position.index_in_line() >= scroll_.get().index_in_line()
    && position.index_in_line() < static_cast<int>(engine_->get_window_widht()) / engine_->get_char_width() + scroll_.get().index_in_line()
    && position.line() < engine_->get_lines_on_screen() + scroll_.get().line();
}

void backbuffer::draw(const position& pivot, const shape::sprite& sh){
    
    const position screen_pivot = global_position_to_buffer_position(pivot);
    
    for(auto rows = sh.data.rowwise();
        auto [line, row] : rows | ranges::views::enumerate)
    {
        for(int ind_in_line{-1}; const char_size ch : row
            | ranges::views::filter([&ind_in_line](const char c){++ind_in_line; return c != shape::whitespace;}))
        {
            if (position p = screen_pivot + position{static_cast<npi_t>(line), ind_in_line}
                ; is_in_buffer(p))
            {
                at(p) = ch;
            } 
        }
    }
}

void backbuffer::erase(const position& pivot, const shape::sprite& sh)
{
    const position screen_pivot = global_position_to_buffer_position(pivot);
    
    for(auto rows = sh.data.rowwise();
        auto [line, row] : rows | ranges::views::enumerate)
    {
        for(int byte_i{-1}; const auto [[maybe_unused]] byte : row
            | ranges::views::filter([&byte_i](const char_size c){++byte_i; return c != shape::whitespace;}))
        {
            if(position p = screen_pivot + position{static_cast<npi_t>(line), byte_i}
                ; is_in_buffer(p))
                {
                    at(p) = shape::whitespace;
                }
                
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


void backbuffer::init()
{
    const int line_lenght = static_cast<int>(engine_->get_window_widht()) / engine_->get_char_width();
    const int lines_count = engine_->get_lines_on_screen();
    
    if(!buffer) buffer = std::make_unique<std::vector< line_type > >(lines_count);
    else buffer->resize(lines_count);
    for(auto& line  : *buffer)
    {
        line.pin().chars.resize(line_lenght + special_chars_count, U' ');
        std::ranges::fill(line.pin().chars, U' ');
        
        *(line.pin().chars.end() - 1/*past-the-end*/ - endl) = U'\n';
        line.pin().chars.back() = U'\0';
    }
    
}

void backbuffer::send()
{
    const auto pos = engine_->get_caret_index();
    bool buffer_is_changed = false;
   
    
    for(int enumerate{-1}; auto& line : *buffer // | ranges::views::enumerate
        | ranges::views::filter([this, &enumerate](auto& l){++enumerate; return l.is_changed() || scroll_.is_changed() ;}))
    {
        
        const npi_t lnum = scroll_.get().line() + enumerate;
        const npi_t fi = engine_->get_first_char_index_in_line(lnum);

        npi_t endsel  = std::max(0, line.get().pasted_bytes_count-1);
        char_size ch_end {'\0'};
        if(lnum >= engine_->get_lines_count()-1)   { ch_end = '\n';   }
        else                                       { endsel  -= endl; }
    
        *(line.pin().chars.end() - 1/*past-the-end*/ - endl) = ch_end; // enable or disable eol


        std::vector<char> bytes{};
        utf8::utf32to8(line.get().chars.begin(), line.get().chars.end(), std::back_inserter(bytes));
        
        engine_->set_selection(fi+scroll_.get().index_in_line(), fi + endsel);
        engine_->replace_selection({bytes.begin(), bytes.end()});
        
        line.pin().pasted_bytes_count = static_cast<int>(bytes.size());
        line.reset_flag();
        buffer_is_changed = true; 
       
        
    }
    
    if(buffer_is_changed){
        engine_->set_caret_index(pos);
        scroll_.reset_flag();
    } 
}

world::world(engine* owner) noexcept: backbuffer(owner), engine_(owner)
{
    
    scroll_changed_connection_ = engine_->get_on_scroll_changed().connect([this](const position& new_scroll)
    {
        redraw_all_actors();
    });

    size_changed_connection = engine_->get_on_resize().connect([this](uint32_t width, uint32_t height){
        redraw_all_actors();
    });
}

world::~world() = default;

void world::redraw_all_actors()
{
    for(const auto view = reg_.view<location_buffer>(); const auto entity : view){
        view.get<location_buffer>(entity).translation.mark_dirty();
    }
}

