


#include <thread>


#include <Windows.h>
#include "world_entities/character.h"
#include "core/common.h"


#include "scintilla/include/Scintilla.h"
#include <memory>



/*
static int m()
{
    
    B b = B{};
    auto& v1 =  b.get_vector();
    //b.get_vector() = std::make_unique<std::vector<int>>();
    v1->push_back(2);
    v1->push_back(10);
    outputlog::mlog().print(v1->at(1));
    return 0;
    
}
*/
static character ch{'f'};
static backbuffer buff{std::shared_ptr<world>()};
DWORD mtick(LPVOID) 
{
    buff.set_world(notepader::get().get_world());
    //buff.init();
    //ch.bind_input();
    for(;;)
    {
      //  outlog::get().print("1");
        //UpdateWindow(notepad::get().get_world()->get_native_window());
        PostMessage(notepader::get().get_main_window(), 99591, 0, 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(10)));
    }
    return 0;
}

void wtick()
{
    
        
           // buff.send();
            on_global_tick();
            //buff.get();  
        
        
        //std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(100)));
        
    
}
DWORD main_post_init_notepad()
{
    
    constexpr double time_step = 1 / 10.0 * 100;
    
    notepader::get().get_input_manager()->init();
    
    
    
    buff.set_world(notepader::get().get_world());
    //buff.init();
    //buff.send();
    ch.bind_input();
    SendMessage(notepader::get().get_world()->get_native_window(), SCI_SETVIRTUALSPACEOPTIONS, SCVS_USERACCESSIBLE|SCVS_NOWRAPLINESTART, 0);
    SendMessage(notepader::get().get_world()->get_native_window(), SCI_SETADDITIONALSELECTIONTYPING, 1, 0);
        
    auto& world = notepader::get().get_world();
    notepader::get().get_world()->show_spaces(true);
    notepader::get().get_world()->show_eol(true);
    
    world->set_background_color(RGB(37,37,38));
    world->set_all_text_color(RGB(240,240,240));
    
    //SendMessage(world->get_native_window(), SCI_HIDESELECTION, 1,0);
    // set font
    SendMessage(world->get_native_window(), SCI_STYLESETFONT, STYLE_DEFAULT, reinterpret_cast<sptr_t>("Lucida Console"));
    SendMessage(world->get_native_window() ,SCI_STYLESETBOLD, STYLE_DEFAULT, 1); // bold
    SendMessage(world->get_native_window(), SCI_STYLESETSIZE, STYLE_DEFAULT,36); // pt size
    SendMessage(world->get_native_window(), SCI_STYLESETCHECKMONOSPACED, STYLE_DEFAULT,1);
    SendMessage(world->get_native_window(), SCI_SETHSCROLLBAR, 1,0);
    

    

    //buff.send();
    //std::string s = std::string(15, 'd');
    //world->append_at_caret_position(s);
    //SendMessage(world->get_native_window(), SCI_SCROLLCARET , 0 , 0);
   

    return 0;
}



BOOL APIENTRY DllMain(const HMODULE h_module, const DWORD  ul_reason_for_call, LPVOID lp_reserved)
{
    // h_module is notepadgame.dll
    // np_module is notepad.exe
    
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    {
        // Ignore thread notifications
        DisableThreadLibraryCalls(h_module);
        
        // run the notepad singleton
        notepader& np = notepader::get();
        
        const auto np_module = GetModuleHandle(nullptr); // get the module handle of the notepad.exe
        
        [[maybe_unused]] constexpr uint8_t start_options = notepader::hide_selection | notepader::kill_focus | notepader::disable_mouse;
        
        np.connect_to_notepad(np_module); // start initialization and a game loop
        
    }

    return TRUE;
}
