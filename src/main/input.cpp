#include <SDL2/SDL.h>
#include "resampler.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include "libretro.h"
#include "io.h"
#include <stdio.h>
#include "imgui.h"
#include <filesystem>
#include "clibretro.h"
#define INI_STRNICMP(s1, s2, cnt) (strcmp(s1, s2))
#include "ini.h"
#include "utils.h"
#include <bitset>

SDL_Joystick *Joystick = NULL;

bool load_inpcfg(retro_input_descriptor *var)
{
    auto lib = CLibretro::get_classinstance();
    lib->core_inputbinds.clear();

    size_t lastindex = lib->core_path.find_last_of(".");
    std::string core_config = lib->core_path.substr(0, lastindex) + ".corecfg";

    while (var->description != NULL && var->port == 0)
    {

        if (var->device == RETRO_DEVICE_ANALOG || (var->device == RETRO_DEVICE_JOYPAD))
        {

            coreinput_bind bind;

            bind.description = var->description;
            auto &settings = bind.config;

            if (var->device == RETRO_DEVICE_ANALOG)
            {
                int var_index = var->index;
                int axistocheck = var->id;
                if ((var_index == RETRO_DEVICE_INDEX_ANALOG_LEFT) && (var->id == RETRO_DEVICE_ID_ANALOG_X))
                    axistocheck = joypad_analogx_l;
                else if ((var_index == RETRO_DEVICE_INDEX_ANALOG_LEFT) && (var->id == RETRO_DEVICE_ID_ANALOG_Y))
                    axistocheck = joypad_analogy_l;
                else if ((var_index == RETRO_DEVICE_INDEX_ANALOG_RIGHT) && (var->id == RETRO_DEVICE_ID_ANALOG_X))
                    axistocheck = joypad_analogx_r;
                else if ((var_index == RETRO_DEVICE_INDEX_ANALOG_RIGHT) && (var->id == RETRO_DEVICE_ID_ANALOG_Y))
                    axistocheck = joypad_analogy_r;

                bind.isanalog = (uint8_t) true;
                bind.retro_id = axistocheck;
                settings.bits.sdl_id = 0;
                settings.bits.joytype = (uint8_t)joytype_::keyboard;
                bind.val = 0;
                bind.joykey_desc = "None";
            }
            else if (var->device == RETRO_DEVICE_JOYPAD)
            {
                bind.isanalog = (uint8_t) false;
                bind.retro_id = var->id;
                settings.bits.sdl_id = 0;
                settings.bits.joytype = (uint8_t)joytype_::keyboard;
                bind.val = 0;
                bind.joykey_desc = "None";
            }
            lib->core_inputbinds.push_back(bind);
            var++;
        }
    }

    unsigned sz_coreconfig = get_filesize(core_config.c_str());
    if (sz_coreconfig)
    {
        unsigned size_;
        std::vector<uint8_t> data = load_data((const char *)core_config.c_str(), &size_);
        ini_t *ini = ini_load((char *)data.data(), NULL);
        int section = ini_find_section(ini, "Input Settings", strlen("Input Settings"));
        if (!section)
        {
        new_sec:
            section = ini_section_add(ini, "Input Settings", strlen("Input Settings"));
            for (auto &bind : lib->core_inputbinds)
            {
                if (bind.description == "")
                    continue;
                std::string val = std::to_string(bind.config.val);
                ini_property_add(ini, section, (char *)bind.description.c_str(), bind.description.length(),
                                 (char *)val.c_str(), val.length());
                std::string keydesc = bind.description + "_keydesc";
                ini_property_add(ini, section, (char *)keydesc.c_str(), keydesc.length(),
                                 (char *)bind.joykey_desc.c_str(), bind.joykey_desc.length());
            }
            std::string numvars = std::to_string(lib->core_inputbinds.size());
            ini_property_add(ini, section, "usedvars_num", strlen("usedvars_num"), numvars.c_str(), numvars.length());
            int size = ini_save(ini, NULL, 0); // Find the size needed
            auto ini_data = std::make_unique<char[]>(size);
            size = ini_save(ini, ini_data.get(), size); // Actually save the file
            save_data((unsigned char *)ini_data.get(), size, core_config.c_str());
            ini_destroy(ini);
            return false;
        }
        else
        {

            int idx = ini_find_property(ini, section, "usedvars_num", strlen("usedvars_num"));
            const char *numvars = ini_property_value(ini, section, idx);
            size_t vars_infile = atoi(numvars);
            if (vars_infile != lib->core_inputbinds.size())
            {
                ini_section_remove(ini, section);
                goto new_sec;
            }
            for (auto &bind : lib->core_inputbinds)
            {
                int idx = ini_find_property(ini, section, bind.description.c_str(),
                                            bind.description.length());
                std::string value = ini_property_value(ini, section, idx);
                bind.config.val = static_cast<uint16_t>(std::stoul(value));
                std::string keydesc = bind.description + "_keydesc";
                int pro1 = ini_find_property(ini, section, (char *)keydesc.c_str(), keydesc.length());
                std::string keyval = ini_property_value(ini, section, pro1);
                bind.joykey_desc = keyval;
            }
        }
        ini_destroy(ini);
    }
    return true;
}
bool save_inpcfg()
{
    auto lib = CLibretro::get_classinstance();
    size_t lastindex = lib->core_path.find_last_of(".");
    std::string core_config = lib->core_path.substr(0, lastindex) + ".corecfg";

    unsigned sz_coreconfig = get_filesize(core_config.c_str());
    if (sz_coreconfig)
    {
        unsigned size_;
        std::vector<uint8_t> data = load_data((const char *)core_config.c_str(), &size_);
        ini_t *ini = ini_load((char *)data.data(), NULL);
        int section = ini_find_section(ini, "Input Settings", strlen("Input"));
        for (auto &bind : lib->core_inputbinds)
        {
            int idx = ini_find_property(ini, section, bind.description.c_str(), bind.description.length());
            std::string value = std::to_string(bind.config.val);
            ini_property_value_set(ini, section, idx, value.c_str(), value.length());
            std::string keydesc = bind.description + "_keydesc";
            int pro1 = ini_find_property(ini, section, (char *)keydesc.c_str(), keydesc.length());
            ini_property_value_set(ini, section, pro1, bind.joykey_desc.c_str(), bind.joykey_desc.length());
        }
        std::string numvars = std::to_string(lib->core_inputbinds.size());
        int idx = ini_find_property(ini, section,
                                    "usedvars_num", strlen("usedvars_num"));
        ini_property_value_set(ini, section, idx, numvars.c_str(),
                               numvars.length());
        int size = ini_save(ini, NULL, 0); // Find the size needed
        auto ini_data = std::make_unique<char[]>(size);
        size = ini_save(ini, ini_data.get(), size); // Actually save the file
        save_data((unsigned char *)ini_data.get(), size, core_config.c_str());
        ini_destroy(ini);
    }
    return true;
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

struct axisarrdig
{
    const char *name;
    int axis;
};

axisarrdig arr_dig[] = {
    {"Left Stick Right", 0},
    {"Left Stick Left", 0},
    {"Left Stick Down", 1},
    {"Left Stick Up", 1},
    {"Right Stick Right", 2},
    {"Right Stick Left", 2},
    {"Right Stick Down", 3},
    {"Right Stick Up", 3},
};

const int Masks[] = {SDL_HAT_UP, SDL_HAT_RIGHT, SDL_HAT_DOWN, SDL_HAT_LEFT};
const char *Names[] = {"Up", "Right", "Down", "Left"};

bool checkbuttons_forui(int selected_inp, bool *isselected_inp)
{
    auto lib = CLibretro::get_classinstance();
    std::string name;

    int axesCount = SDL_JoystickNumAxes(Joystick);
    auto &bind = lib->core_inputbinds[selected_inp];
    for (int a = 0; a < axesCount; a++)
    {

        if (bind.isanalog)
        {
            Sint16 axis = 0;
            if (a > 4)
                return false;
            axis = SDL_JoystickGetAxis(Joystick, a);
            const int JOYSTICK_DEAD_ZONE = 0x1000;
            if (axis < -JOYSTICK_DEAD_ZONE || axis > JOYSTICK_DEAD_ZONE)
            {
                if (a < 4)
                {
                    name = axis_arr[a];
                    bind.joykey_desc = name;
                    bind.config.bits.sdl_id = a;
                    bind.val = 0;
                    bind.config.bits.joytype = joytype_::joystick_;
                    *isselected_inp = false;
                    ImGui::SetWindowFocus(NULL);
                    return true;
                }
            }
        }
        else
        {
            Sint16 axis = SDL_JoystickGetAxis(Joystick, a);
            const int JOYSTICK_DEAD_ZONE = 0x4000;
            if (axis < -JOYSTICK_DEAD_ZONE || axis > JOYSTICK_DEAD_ZONE)
            {
                for (int i = 0; i < 7; i++)
                {
                    if (a == arr_dig[i].axis)
                    {
                        bind.joykey_desc = (axis > JOYSTICK_DEAD_ZONE) ? arr_dig[i].name : arr_dig[i + 1].name;
                        bind.config.bits.sdl_id = a;
                        bind.val = 0;
                        bind.config.bits.joytype = joytype_::joystick_;
                        *isselected_inp = false;
                        ImGui::SetWindowFocus(NULL);
                        return true;
                    }
                    if (a >= 4)
                    {
                        if (axis > JOYSTICK_DEAD_ZONE)
                        {
                            bind.joykey_desc = (a > 4) ? "R Trigger" : "L Trigger";
                            bind.config.bits.sdl_id = a;
                            bind.val = 0;
                            bind.config.bits.joytype = joytype_::joystick_;
                            *isselected_inp = false;
                            ImGui::SetWindowFocus(NULL);
                            return true;
                        }
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
            bind.config.bits.sdl_id = b;
            bind.val = 0;
            bind.config.bits.joytype = joytype_::button;
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
        for (size_t i = 0; i < sizeof(Masks) / sizeof(Masks[0]); i++)
        {
            if (hat & Masks[i])
            {
                name = "Hat ";
                name += Names[i];
                bind.joykey_desc = name;
                bind.config.bits.sdl_id = hat;
                bind.val = 0;
                bind.config.bits.joytype = joytype_::hat;
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
        if (!SDL_JoystickGetAttached(Joystick))
        {
            close_inp();
            init_inp();
            SDL_JoystickUpdate();
        }
        return checkbuttons_forui(selected_inp, isselected_inp);
    }
    else
        return false;
}

void poll_lr()
{
    auto lib = CLibretro::get_classinstance();
    SDL_JoystickUpdate();
    if (!SDL_JoystickGetAttached(Joystick))
    {
        close_inp();
        init_inp();
        SDL_JoystickUpdate();
    }

    for (auto &bind : lib->core_inputbinds)
    {
        if (bind.config.bits.joytype == joytype_::joystick_)
        {

            Sint16 axis = SDL_JoystickGetAxis(Joystick, bind.config.bits.sdl_id);
            const int JOYSTICK_DEAD_ZONE = 0x4000;
            if (bind.config.bits.sdl_id < 4)
            {
                if (axis < -JOYSTICK_DEAD_ZONE || axis > JOYSTICK_DEAD_ZONE)
                    bind.val = (bind.isanalog) ? axis : 1;
                else
                    bind.val = 0;
            }
            else
            {
                if (axis > JOYSTICK_DEAD_ZONE)
                    bind.val = (bind.isanalog) ? axis : 1;
                else
                    bind.val = 0;
            }
        }
        else if (bind.config.bits.joytype == joytype_::hat)
            bind.val = SDL_JoystickGetHat(Joystick, 0) & bind.config.bits.sdl_id;
        else if (bind.config.bits.joytype == joytype_::button)
            bind.val = SDL_JoystickGetButton(Joystick, bind.config.bits.sdl_id);
        else if (bind.config.bits.joytype == joytype_::keyboard)
            continue;
    }
}

int16_t input_state(unsigned port, unsigned device, unsigned index,
                    unsigned id)
{
    if (port != 0)
        return 0;
    auto lib = CLibretro::get_classinstance();

    if (device == RETRO_DEVICE_MOUSE)
    {
        return 0;
    }

    if (device == RETRO_DEVICE_ANALOG)
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
        for (auto &bind : lib->core_inputbinds)
        {
            if (bind.retro_id == axistocheck && bind.isanalog)
                return bind.val;
        }
    }
    else if (device == RETRO_DEVICE_JOYPAD)
    {
        for (auto &bind : lib->core_inputbinds)
        {
            if (bind.retro_id == id && !bind.isanalog)
                return bind.val;
        }
    }
    return 0;
}
