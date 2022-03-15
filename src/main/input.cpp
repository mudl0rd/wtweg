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

const char *axis_arr_digital[] =
    {
        "Left Stick Right",
        "Left Stick Left",
        "Left Stick Down",
        "Left Stick Up",
        "Right Stick Right",
        "Right Stick Left",
        "Right Stick Down",
        "Right Stick Up",
        "L Trigger",
        "R Trigger"};

const int Masks[] = {SDL_HAT_UP, SDL_HAT_RIGHT, SDL_HAT_DOWN, SDL_HAT_LEFT};
const char *Names[] = {"Up", "Right", "Down", "Left"};

bool checkbuttons_forui(int selected_inp, bool *isselected_inp)
{
    CLibretro *lib = CLibretro::get_classinstance();
    std::string name;
    SDL_JoystickUpdate();
    int axesCount = SDL_JoystickNumAxes(Joystick);
    for (int a = 0; a < axesCount; a++)
    {
        Sint16 axis = SDL_JoystickGetAxis(Joystick, a);
        if (lib->core_inputbinds[selected_inp].isanalog)
        {

            if (axis <= -0x8000 || (axis >= 0x8000))
            {

                name = axis_arr[a];
                if (a < 4)
                {
                    if (axis < -0x4000)
                        name += "-";
                    else if (axis > 0x4000)
                        name += "+";
                    else
                        return false;
                }
                lib->core_inputbinds[selected_inp].joykey_desc = name;
                lib->core_inputbinds[selected_inp].sdl_id = a;
                lib->core_inputbinds[selected_inp].joytype = joytype_::joystick_;
                *isselected_inp = false;
                ImGui::SetWindowFocus(NULL);
                return true;
            }
        }
        else
        {

            const int JOYSTICK_DEAD_ZONE = 4000;
            if (axis < -JOYSTICK_DEAD_ZONE || axis > JOYSTICK_DEAD_ZONE)
            {

                if (a == 0)
                {

                    if (axis > 0x4000)
                    {
                        lib->core_inputbinds[selected_inp].joykey_desc = axis_arr_digital[0];
                        lib->core_inputbinds[selected_inp].ispos = true;
                        *isselected_inp = false;
                        ImGui::SetWindowFocus(NULL);
                        return true;
                    }
                    if (axis < -0x4000)
                    {
                        lib->core_inputbinds[selected_inp].joykey_desc = axis_arr_digital[1];
                        lib->core_inputbinds[selected_inp].ispos = false;
                        *isselected_inp = false;
                        ImGui::SetWindowFocus(NULL);
                        return true;
                    }
                }

                if (a == 1)
                {
                    if (axis >= 0x4000)
                    {
                        lib->core_inputbinds[selected_inp].joykey_desc = axis_arr_digital[2];
                        lib->core_inputbinds[selected_inp].ispos = true;
                        *isselected_inp = false;
                        ImGui::SetWindowFocus(NULL);
                        return true;
                    }
                    else if (axis <= -0x4000)
                    {
                        lib->core_inputbinds[selected_inp].joykey_desc = axis_arr_digital[3];
                        lib->core_inputbinds[selected_inp].ispos = false;
                        *isselected_inp = false;
                        ImGui::SetWindowFocus(NULL);
                        return true;
                    }
                }

                if (a == 2)
                {
                    if (axis >= 0x4000)
                    {
                        lib->core_inputbinds[selected_inp].joykey_desc = axis_arr_digital[4];
                        lib->core_inputbinds[selected_inp].ispos = true;
                        goto thisisahack;
                    }
                    else if (axis <= -0x4000)
                    {
                        lib->core_inputbinds[selected_inp].joykey_desc = axis_arr_digital[5];
                        lib->core_inputbinds[selected_inp].ispos = false;
                        goto thisisahack;
                    }
                }
                if (a == 3)
                {
                    if (axis >= 0x4000)
                    {
                        lib->core_inputbinds[selected_inp].joykey_desc = axis_arr_digital[6];
                        lib->core_inputbinds[selected_inp].ispos = true;
                        goto thisisahack;
                    }
                    else if (axis <= -0x4000)
                    {
                        lib->core_inputbinds[selected_inp].joykey_desc = axis_arr_digital[7];
                        lib->core_inputbinds[selected_inp].ispos = false;
                        goto thisisahack;
                    }
                }
                if (a == 4)
                {
                    if (axis >= 0x4000)
                    {
                        lib->core_inputbinds[selected_inp].joykey_desc = axis_arr_digital[8];
                        lib->core_inputbinds[selected_inp].ispos = true;
                        lib->core_inputbinds[selected_inp].joytype = joytype_::joystick_;
                        goto thisisahack;
                    }
                }
                if (a == 5)
                {
                    if (axis >= 0x4000)
                    {
                        lib->core_inputbinds[selected_inp].joykey_desc = axis_arr_digital[9];
                        lib->core_inputbinds[selected_inp].ispos = true;
                        lib->core_inputbinds[selected_inp].joytype = joytype_::joystick_;
                    thisisahack:
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
            lib->core_inputbinds[selected_inp].joykey_desc = name;
            lib->core_inputbinds[selected_inp].sdl_id = b;
            lib->core_inputbinds[selected_inp].joytype = joytype_::button;
            *isselected_inp = false;
            ImGui::SetWindowFocus(NULL);
            return true;
        }
    }

    int hatsCount = SDL_JoystickNumHats(Joystick);

    for (int h = 0; h < hatsCount; h++)
    {
        int hat = SDL_JoystickGetHat(Joystick, h);
        for (int i = 0; i < sizeof(Masks) / sizeof(Masks[0]); i++)
        {
            if (hat & Masks[i])
            {
                name = "Hat ";
                name += Names[i];

                lib->core_inputbinds[selected_inp].joykey_desc = name;
                lib->core_inputbinds[selected_inp].sdl_id = hat;
                lib->core_inputbinds[selected_inp].joytype = joytype_::hat;
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
    CLibretro *lib = CLibretro::get_classinstance();

    if (*isselected_inp)
    {
        return checkbuttons_forui(selected_inp, isselected_inp);
    }
    else
    {
        SDL_JoystickUpdate();
        for (int i = 0; i < lib->core_inputbinds.size(); i++)
        {
            if (lib->core_inputbinds[i].joytype == joytype_::joystick_)
            {
                int axesCount = SDL_JoystickNumAxes(Joystick);
                if (lib->core_inputbinds[i].isanalog)
                {
                    for (int a = 0; a < axesCount; a++)
                    {
                        int axis = SDL_JoystickGetAxis(Joystick, a);
                        const int JOYSTICK_DEAD_ZONE = 4000;
                        if (axis < -JOYSTICK_DEAD_ZONE || axis > JOYSTICK_DEAD_ZONE)
                        {
                            if (lib->core_inputbinds[i].sdl_id == a)
                            {
                                lib->core_inputbinds[i].val = axis;
                                lib->core_inputbinds[i].pressed = true;
                            }
                        }
                    }
                }
                else
                {
                    for (int a = 0; a < axesCount; a++)
                    {
                        int axis = SDL_JoystickGetAxis(Joystick, a);
                        const int JOYSTICK_DEAD_ZONE = 4000;
                        if (axis < -JOYSTICK_DEAD_ZONE || axis > JOYSTICK_DEAD_ZONE)
                        {
                            if (lib->core_inputbinds[i].sdl_id == a)
                            {
                                bool ispos = axis > 0x4000;
                                if (lib->core_inputbinds[i].ispos == ispos)
                                    lib->core_inputbinds[i].pressed = true;
                            }
                        }
                    }
                }
            }

            if (lib->core_inputbinds[i].joytype == joytype_::hat)
            {
                int hatsCount = SDL_JoystickNumHats(Joystick);
                if (hatsCount != 1)
                    return true;
                // get first hat.
                int hat = SDL_JoystickGetHat(Joystick, 0);
                lib->core_inputbinds[i].pressed = hat & lib->core_inputbinds[i].sdl_id;
            }

            if (lib->core_inputbinds[i].joytype == joytype_::button)
            {
                int buttonsCount = SDL_JoystickNumButtons(Joystick);
                for (int b = 0; b < buttonsCount; b++)
                {
                    if (lib->core_inputbinds[i].sdl_id == b)
                    {
                        int btn = SDL_JoystickGetButton(Joystick, b);
                        lib->core_inputbinds[i].pressed = btn;
                    }
                }
            }
        }
    }
}

void poll_lr()
{
    SDL_JoystickUpdate();
}

int input_state(unsigned port, unsigned device, unsigned index,
                unsigned id)
{
    if (port != 0)
        return 0;
    CLibretro *lib = CLibretro::get_classinstance();

    if (device == RETRO_DEVICE_MOUSE)
    {
        return 0;
    }

    if (device == RETRO_DEVICE_ANALOG || device == RETRO_DEVICE_JOYPAD)
    {
        int var_index = index;
        int axistocheck = id;
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
            int retro_id = lib->core_inputbinds[i].retro_id;
            bool isanalog = lib->core_inputbinds[i].isanalog;
            if (retro_id == id)
            {
                if (lib->core_inputbinds[i].pressed)
                {
                    if (isanalog)
                        return lib->core_inputbinds[i].val;
                    return 1;
                }
                return 0;
            }
        }
        return 0;
    }
}
