#include <SDL2/SDL.h>
#include "resampler.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include "libretro.h"
#include "io.h"
#include <stdio.h>
#include "sdlggerat.h"
#include "imgui.h"
#include <filesystem>
#include "clibretro.h"

SDL_Joystick *Joystick = NULL;

bool load_inpcfg()
{
}
bool save_inpcfg()
{
}
void close_inp()
{
    if (Joystick)
    {
        SDL_JoystickClose(Joystick);
        Joystick = nullptr;
    }
}
void init_inp()
{
    if (Joystick)
        SDL_JoystickClose(Joystick);

    int num = SDL_NumJoysticks();
    if (num < 1)
    {
        Joystick = nullptr;
        return;
    }
    Joystick = SDL_JoystickOpen(0);
}

const char *axis_arr[] =
    {
        "Left Stick X",
        "Left Stick Y",
        "Right Stick X",
        "Right Stick Y",
        "L Trigger",
        "R Trigger"};


struct axisarrdig {
    char* name;
    int axis;
};

axisarrdig arr_dig[] = {
       { "Left Stick Right",0},
       { "Left Stick Left",0},
       { "Left Stick Down",1},
       { "Left Stick Up",1},
       { "Right Stick Right",2},
       { "Right Stick Left",2},
       {"Right Stick Down",3},
       { "Right Stick Up",3},
       { "L Trigger",4},
       { "R Trigger",4}
};

const int Masks[] = {SDL_HAT_UP, SDL_HAT_RIGHT, SDL_HAT_DOWN, SDL_HAT_LEFT};
const char *Names[] = {"Up", "Right", "Down", "Left"};

bool checkbuttons_forui(int selected_inp, bool *isselected_inp)
{
    CLibretro *lib = CLibretro::get_classinstance();
    std::string name;
   
    int axesCount = SDL_JoystickNumAxes(Joystick);
    auto &bind = lib->core_inputbinds[selected_inp];
    for (int a = 0; a < axesCount; a++)
    {
        
        if (bind.isanalog)
        {
            Sint16 axis = 0;
            if(a > 4)return false;
            axis = SDL_JoystickGetAxis(Joystick, a);
            const int JOYSTICK_DEAD_ZONE = 0x1000;
            if (axis < -JOYSTICK_DEAD_ZONE || axis > JOYSTICK_DEAD_ZONE)
            {
                if (a < 4)
                {
                name = axis_arr[a];
                bind.joykey_desc = name;
                bind.sdl_id = a;
                bind.joytype = joytype_::joystick_;
                *isselected_inp = false;
                ImGui::SetWindowFocus(NULL);
                return true;
                }
                
            }
        }
        else
        {
            Sint16 axis = SDL_JoystickGetAxis(Joystick, a);
            const int JOYSTICK_DEAD_ZONE = 0x1000;
            if (axis < -JOYSTICK_DEAD_ZONE || axis > JOYSTICK_DEAD_ZONE)
            {
                for(int i=0;i<5;i++)
                {
                    if(a==arr_dig[i].axis)
                    {
                        if(axis > JOYSTICK_DEAD_ZONE)
                        {
                         bind.ispos = true;
                        bind.joykey_desc = arr_dig[i+1].name;
                        }
                        else
                        {
                        bind.ispos = false;
                        bind.joykey_desc = arr_dig[i].name;
                        }
                        bind.sdl_id = a;
                        bind.joytype = joytype_::joystick_;
                        *isselected_inp = false;
                        ImGui::SetWindowFocus(NULL);
                        return true;
                    }
                }       
            }
        }
    }

    int buttonsCount = SDL_JoystickNumButtons(Joystick);

    for (int b = 0; b < buttonsCount; b++)
    {
        int btn = SDL_JoystickGetButton(Joystick, b);

        if (btn)
        {
            name += "Button " + std::to_string(b);
            bind.joykey_desc = name;
            bind.sdl_id = b;
            bind.joytype = joytype_::button;
            *isselected_inp = false;
            ImGui::SetWindowFocus(NULL);
            return true;
        }
    }

    int hatsCount = SDL_JoystickNumHats(Joystick);
     if (hatsCount != 1)
                    return true;

    for (int h = 0; h < hatsCount; h++)
    {
        int hat = SDL_JoystickGetHat(Joystick, h);
        for (int i = 0; i < sizeof(Masks) / sizeof(Masks[0]); i++)
        {
            if (hat & Masks[i])
            {
                name = "Hat ";
                name += Names[i];
                bind.joykey_desc = name;
                bind.sdl_id = hat;
                bind.joytype = joytype_::hat;
                *isselected_inp = false;
                ImGui::SetWindowFocus(NULL);
                return true;
            }
        }
    }
    return true;
}

bool poll_inp(int selected_inp, bool *isselected_inp)
{
    if (*isselected_inp)
    {
        SDL_JoystickUpdate();
        return checkbuttons_forui(selected_inp, isselected_inp);
    }
    else return false;
}

void poll_lr()
{
    extern bool closed_dialog;
    if (closed_dialog)
    {
    if(!SDL_JoystickGetAttached(Joystick))
    {
        close_inp();
        init_inp();
        SDL_JoystickUpdate();
    }
    
    CLibretro *lib = CLibretro::get_classinstance();
    SDL_LockJoysticks();
    SDL_JoystickUpdate();
        for (int i = 0; i < lib->core_inputbinds.size(); i++)
        {
            auto &bind = lib->core_inputbinds[i];
            
            if (bind.joytype == joytype_::joystick_)
            {
                       
                        Sint16 axis = SDL_JoystickGetAxis(Joystick, bind.sdl_id);
                        const char* err = SDL_GetError();
                             if (bind.isanalog){
                                const int JOYSTICK_DEAD_ZONE = 0x4000;
                                if (axis < -JOYSTICK_DEAD_ZONE || axis > JOYSTICK_DEAD_ZONE)
                                 bind.val = axis;
                                 else bind.val = 0;
                             }
                             else
                            {
                                const int JOYSTICK_DEAD_ZONE = 0x4000;
                                if (axis < -JOYSTICK_DEAD_ZONE || axis > JOYSTICK_DEAD_ZONE)
                                {
                                 bool ispos = axis > JOYSTICK_DEAD_ZONE;
                                 bind.val = ispos? 1:0;
                                }
                                else
                                bind.val = 0;
                            }  
            }
            else if (bind.joytype == joytype_::hat)
                bind.val = SDL_JoystickGetHat(Joystick, 0) & bind.sdl_id;
            else if (bind.joytype == joytype_::button)
                    bind.val = SDL_JoystickGetButton(Joystick, bind.sdl_id);
             else if (bind.joytype == joytype_::keyboard) continue;
        }
    }
     SDL_UnlockJoysticks();
}

int16_t input_state(unsigned port, unsigned device, unsigned index,
                unsigned id)
{
    if (port != 0)
        return 0;
    CLibretro *lib = CLibretro::get_classinstance();

    if (device == RETRO_DEVICE_MOUSE)
    {
        return 0;
    }
    
    if(device == RETRO_DEVICE_ANALOG)
    {
       int axistocheck = id;
       int var_index = index;
       if ((var_index == RETRO_DEVICE_INDEX_ANALOG_LEFT) && (id == RETRO_DEVICE_ID_ANALOG_X))
          axistocheck = joypad_analogx_l;
        else if ((var_index == RETRO_DEVICE_INDEX_ANALOG_LEFT) && (id == RETRO_DEVICE_ID_ANALOG_Y))
          axistocheck = joypad_analogy_l;
        else if ((var_index == RETRO_DEVICE_INDEX_ANALOG_RIGHT) && (id == RETRO_DEVICE_ID_ANALOG_X))
          axistocheck = joypad_analogx_r;
        else if ((var_index == RETRO_DEVICE_INDEX_ANALOG_RIGHT) && (id == RETRO_DEVICE_ID_ANALOG_Y))
          axistocheck = joypad_analogy_r;
        for (unsigned int i = 0; i < lib->core_inputbinds.size(); i++)
        {
            auto bind = lib->core_inputbinds[i];
            if(bind.retro_id == axistocheck && bind.isanalog)
                    return lib->core_inputbinds[i].val;
        }
    }
    else if (device == RETRO_DEVICE_JOYPAD)
    {
        for (unsigned int i = 0; i < lib->core_inputbinds.size(); i++)
        {
            auto bind = lib->core_inputbinds[i];
                if(bind.retro_id == id && !bind.isanalog)
                    return bind.val;
        }
    }
    return 0;
}
