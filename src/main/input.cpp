#include <SDL2/SDL.h>
#include <SDL2/SDL_mouse.h>
#include "libretro.h"
#include "inout.h"
#include <stdio.h>
#include "imgui.h"
#include <filesystem>
#include "clibretro.h"
#define INI_STRNICMP(s1, s2, cnt) (strcmp(s1, s2))
#include "ini.h"
#include "mudutils/utils.h"
#include <bitset>
#include <array>

struct key_map
{
    SDL_Keycode sym;
    enum retro_key rk;
};

struct mousiebind
{
    int rel_x;
    int rel_y;
    int abs_x;
    int abs_y;
    int l;
    int r;
    int m;
    int b4;
    int b5;
    int wu;
    int wd;
};
mousiebind mousiez = {0};
const Uint8 *lr_keymap;

const struct key_map key_map_[] = {
    {SDLK_BACKSPACE, RETROK_BACKSPACE},
    {SDLK_TAB, RETROK_TAB},
    {SDLK_CLEAR, RETROK_CLEAR},
    {SDLK_RETURN, RETROK_RETURN},
    {SDLK_PAUSE, RETROK_PAUSE},
    {SDLK_ESCAPE, RETROK_ESCAPE},
    {SDLK_SPACE, RETROK_SPACE},
    {SDLK_EXCLAIM, RETROK_EXCLAIM},
    {SDLK_QUOTEDBL, RETROK_QUOTEDBL},
    {SDLK_HASH, RETROK_HASH},
    {SDLK_DOLLAR, RETROK_DOLLAR},
    {SDLK_AMPERSAND, RETROK_AMPERSAND},
    {SDLK_QUOTE, RETROK_QUOTE},
    {SDLK_LEFTPAREN, RETROK_LEFTPAREN},
    {SDLK_RIGHTPAREN, RETROK_RIGHTPAREN},
    {SDLK_ASTERISK, RETROK_ASTERISK},
    {SDLK_PLUS, RETROK_PLUS},
    {SDLK_COMMA, RETROK_COMMA},
    {SDLK_MINUS, RETROK_MINUS},
    {SDLK_PERIOD, RETROK_PERIOD},
    {SDLK_SLASH, RETROK_SLASH},
    {SDLK_0, RETROK_0},
    {SDLK_1, RETROK_1},
    {SDLK_2, RETROK_2},
    {SDLK_3, RETROK_3},
    {SDLK_4, RETROK_4},
    {SDLK_5, RETROK_5},
    {SDLK_6, RETROK_6},
    {SDLK_7, RETROK_7},
    {SDLK_8, RETROK_8},
    {SDLK_9, RETROK_9},
    {SDLK_COLON, RETROK_COLON},
    {SDLK_SEMICOLON, RETROK_SEMICOLON},
    {SDLK_LESS, RETROK_OEM_102},
    {SDLK_EQUALS, RETROK_EQUALS},
    {SDLK_GREATER, RETROK_GREATER},
    {SDLK_QUESTION, RETROK_QUESTION},
    {SDLK_AT, RETROK_AT},
    {SDLK_LEFTBRACKET, RETROK_LEFTBRACKET},
    {SDLK_BACKSLASH, RETROK_BACKSLASH},
    {SDLK_RIGHTBRACKET, RETROK_RIGHTBRACKET},
    {SDLK_CARET, RETROK_CARET},
    {SDLK_UNDERSCORE, RETROK_UNDERSCORE},
    {SDLK_BACKQUOTE, RETROK_BACKQUOTE},
    {SDLK_a, RETROK_a},
    {SDLK_b, RETROK_b},
    {SDLK_c, RETROK_c},
    {SDLK_d, RETROK_d},
    {SDLK_e, RETROK_e},
    {SDLK_f, RETROK_f},
    {SDLK_g, RETROK_g},
    {SDLK_h, RETROK_h},
    {SDLK_i, RETROK_i},
    {SDLK_j, RETROK_j},
    {SDLK_k, RETROK_k},
    {SDLK_l, RETROK_l},
    {SDLK_m, RETROK_m},
    {SDLK_n, RETROK_n},
    {SDLK_o, RETROK_o},
    {SDLK_p, RETROK_p},
    {SDLK_q, RETROK_q},
    {SDLK_r, RETROK_r},
    {SDLK_s, RETROK_s},
    {SDLK_t, RETROK_t},
    {SDLK_u, RETROK_u},
    {SDLK_v, RETROK_v},
    {SDLK_w, RETROK_w},
    {SDLK_x, RETROK_x},
    {SDLK_y, RETROK_y},
    {SDLK_z, RETROK_z},
    {SDLK_DELETE, RETROK_DELETE},
    {SDLK_KP_0, RETROK_KP0},
    {SDLK_KP_1, RETROK_KP1},
    {SDLK_KP_2, RETROK_KP2},
    {SDLK_KP_3, RETROK_KP3},
    {SDLK_KP_4, RETROK_KP4},
    {SDLK_KP_5, RETROK_KP5},
    {SDLK_KP_6, RETROK_KP6},
    {SDLK_KP_7, RETROK_KP7},
    {SDLK_KP_8, RETROK_KP8},
    {SDLK_KP_9, RETROK_KP9},
    {SDLK_KP_PERIOD, RETROK_KP_PERIOD},
    {SDLK_KP_DIVIDE, RETROK_KP_DIVIDE},
    {SDLK_KP_MULTIPLY, RETROK_KP_MULTIPLY},
    {SDLK_KP_MINUS, RETROK_KP_MINUS},
    {SDLK_KP_PLUS, RETROK_KP_PLUS},
    {SDLK_KP_ENTER, RETROK_KP_ENTER},
    {SDLK_KP_EQUALS, RETROK_KP_EQUALS},
    {SDLK_UP, RETROK_UP},
    {SDLK_DOWN, RETROK_DOWN},
    {SDLK_RIGHT, RETROK_RIGHT},
    {SDLK_LEFT, RETROK_LEFT},
    {SDLK_INSERT, RETROK_INSERT},
    {SDLK_HOME, RETROK_HOME},
    {SDLK_END, RETROK_END},
    {SDLK_PAGEUP, RETROK_PAGEUP},
    {SDLK_PAGEDOWN, RETROK_PAGEDOWN},
    {SDLK_F1, RETROK_F1},
    {SDLK_F2, RETROK_F2},
    {SDLK_F3, RETROK_F3},
    {SDLK_F4, RETROK_F4},
    {SDLK_F5, RETROK_F5},
    {SDLK_F6, RETROK_F6},
    {SDLK_F7, RETROK_F7},
    {SDLK_F8, RETROK_F8},
    {SDLK_F9, RETROK_F9},
    {SDLK_F10, RETROK_F10},
    {SDLK_F11, RETROK_F11},
    {SDLK_F12, RETROK_F12},
    {SDLK_F13, RETROK_F13},
    {SDLK_F14, RETROK_F14},
    {SDLK_F15, RETROK_F15},
    {SDLK_NUMLOCKCLEAR, RETROK_NUMLOCK},
    {SDLK_CAPSLOCK, RETROK_CAPSLOCK},
    {SDLK_SCROLLLOCK, RETROK_SCROLLOCK},
    {SDLK_RSHIFT, RETROK_RSHIFT},
    {SDLK_LSHIFT, RETROK_LSHIFT},
    {SDLK_RCTRL, RETROK_RCTRL},
    {SDLK_LCTRL, RETROK_LCTRL},
    {SDLK_RALT, RETROK_RALT},
    {SDLK_LALT, RETROK_LALT},
    /* { ?, RETROK_RMETA }, */
    /* { ?, RETROK_LMETA }, */
    {SDLK_LGUI, RETROK_LSUPER},
    {SDLK_RGUI, RETROK_RSUPER},
    {SDLK_MODE, RETROK_MODE},
    {SDLK_HELP, RETROK_HELP},
    {SDLK_PRINTSCREEN, RETROK_PRINT},
    {SDLK_SYSREQ, RETROK_SYSREQ},
    {SDLK_PAUSE, RETROK_BREAK},
    {SDLK_MENU, RETROK_MENU},
    {SDLK_POWER, RETROK_POWER},

    {SDLK_UNDO, RETROK_UNDO},

    {0, RETROK_UNKNOWN},
};
std::vector<SDL_GameController *> Joystick = {};

int axistocheck(int id, int index)
{
    switch (index)
    {
    case RETRO_DEVICE_INDEX_ANALOG_LEFT:
        return (id == RETRO_DEVICE_ID_ANALOG_X) ? joypad_analogx_l : joypad_analogy_l;
        break;
    case RETRO_DEVICE_INDEX_ANALOG_RIGHT:
        return (id == RETRO_DEVICE_ID_ANALOG_X) ? joypad_analogx_r : joypad_analogy_r;
        break;
    case RETRO_DEVICE_INDEX_ANALOG_BUTTON:
        switch (id)
        {
        case RETRO_DEVICE_ID_JOYPAD_L2:
            return joypad_analog_l2;
            break;
        case RETRO_DEVICE_ID_JOYPAD_R2:
            return joypad_analog_r2;
            break;
        case RETRO_DEVICE_ID_JOYPAD_L3:
            return joypad_analog_l3;
            break;
        case RETRO_DEVICE_ID_JOYPAD_R3:
            return joypad_analog_r3;
            break;
        }
        break;
    }
}

bool checkjs(int port)
{
    return (Joystick.size() && Joystick[port]) ? SDL_GameControllerGetAttached(Joystick[port]) : false;
}

bool loadcontconfig(bool save_f)
{
    auto lib = CLibretro::get_classinstance();

    if (!save_f)
    {
        for (auto &i : lib->core_inpbinds)
        {
            size_t k = &i - &lib->core_inpbinds.front();
            for (auto &inp_cfg : lib->core_inputttypes)
            {
                size_t l = &inp_cfg - &lib->core_inputttypes.front();
                if (k == l)
                {
                    i.controlinfo = inp_cfg;
                    i.controller_type = RETRO_DEVICE_JOYPAD;
                }
            }
        }
    }

    std::string core_config = lib->core_config;
    unsigned sz_coreconfig = MudUtil::get_filesize(core_config.c_str());
    std::vector<uint8_t> data = MudUtil::load_data((const char *)core_config.c_str());
    ini_t *ini = (!sz_coreconfig) ? ini_create(NULL) : ini_load((char *)data.data(), NULL);
    int portage = 0;
    std::string section_desc = "contpt_" + std::to_string(lib->config_crc);
    int section = ini_find_section(ini, section_desc.c_str(), section_desc.length());
    auto save = [=]()
    {
        int size = ini_save(ini, NULL, 0); // Find the size needed
        auto ini_data = std::make_unique<char[]>(size);
        size = ini_save(ini, ini_data.get(), size); // Actually save the file
        MudUtil::save_data((unsigned char *)ini_data.get(), size, core_config.c_str());
    };

    if (section == INI_NOT_FOUND)
    {
        // not found, add section
        section = ini_section_add(ini, section_desc.c_str(), section_desc.length());
        auto save_conf = [=](std::string keydesc, std::string value_str)
        {
            int idx = ini_find_property(ini, section, keydesc.c_str(),
                                        keydesc.length());
            if (idx == INI_NOT_FOUND)
                ini_property_add(ini, section, keydesc.c_str(), keydesc.length(),
                                 value_str.c_str(), value_str.length());
        };
        for (auto &controller : lib->core_inpbinds)
        {
            std::string key = "controller_type" + std::to_string(portage++);
            save_conf(key, std::to_string(controller.controller_type));
        }
        save();
        ini_destroy(ini);
        return true;
    }
    portage = 0;

    for (auto &controller : lib->core_inpbinds)
    {
        if (!save_f)
        {
            auto load_conf = [=](std::string keydesc, std::string value_str)
            {
                int idx = ini_find_property(ini, section, keydesc.c_str(),
                                            keydesc.length());
                if (idx == INI_NOT_FOUND)
                {
                    ini_property_add(ini, section, keydesc.c_str(), keydesc.length(),
                                     value_str.c_str(), value_str.length());
                    save();
                    return value_str;
                }
                std::string str = ini_property_value(ini, section, idx);
                return str;
            };
            std::string key = "controller_type" + std::to_string(portage);
            controller.controller_type = static_cast<int32_t>(std::stoi(load_conf(key,
                                                                                  std::to_string(controller.controller_type))));
        }
        else
        {
            std::string key = "controller_type" + std::to_string(portage++);
            auto save_conf = [=](std::string keydesc, std::string value_str)
            {
                int idx = ini_find_property(ini, section, keydesc.c_str(), keydesc.length());
                (idx == INI_NOT_FOUND) ? ini_property_add(ini, section, keydesc.c_str(), keydesc.length(),
                                                          value_str.c_str(), value_str.length())
                                       : ini_property_value_set(ini, section, idx, value_str.c_str(), value_str.length());
            };
            save_conf(key, std::to_string(controller.controller_type));
        }
        portage++;
    }
    if (save_f)
        save();
    ini_destroy(ini);
    return true;
}

bool loadinpconf(uint32_t checksum, bool save_f)
{
    auto lib = CLibretro::get_classinstance();
    std::string core_config = lib->core_config;
    unsigned sz_coreconfig = MudUtil::get_filesize(core_config.c_str());
    std::vector<uint8_t> data = MudUtil::load_data((const char *)core_config.c_str());
    ini_t *ini = NULL;
    int portage = 0;

    ini = (!sz_coreconfig) ? ini_create(NULL) : ini_load((char *)data.data(), NULL);

    auto save = [=]()
    {
        int size = ini_save(ini, NULL, 0); // Find the size needed
        auto ini_data = std::make_unique<char[]>(size);
        size = ini_save(ini, ini_data.get(), size); // Actually save the file
        MudUtil::save_data((unsigned char *)ini_data.get(), size, core_config.c_str());
    };

    for (auto &controller : lib->core_inpbinds)
    {
        std::string section_desc = "P" + std::to_string(portage++) + "_" + std::to_string(checksum);
        int section = ini_find_section(ini, section_desc.c_str(), section_desc.length());
        if (section == INI_NOT_FOUND)
            section = ini_section_add(ini, section_desc.c_str(), section_desc.length());
        auto load_conf = [=](std::string keydesc, std::string value_str)
        {
            int idx = ini_find_property(ini, section, keydesc.c_str(),
                                        keydesc.length());
            if (idx == INI_NOT_FOUND)
            {
                ini_property_add(ini, section, keydesc.c_str(), keydesc.length(),
                                 value_str.c_str(), value_str.length());
                save();
                return value_str;
            }
            std::string str = ini_property_value(ini, section, idx);
            return str;
        };
        auto save_conf = [=](std::string keydesc, std::string value_str)
        {
            int idx = ini_find_property(ini, section, keydesc.c_str(), keydesc.length());
            if (idx == INI_NOT_FOUND)
            {
                ini_property_add(ini, section, keydesc.c_str(), keydesc.length(),
                                 value_str.c_str(), value_str.length());
            }
            else
                ini_property_value_set(ini, section, idx, value_str.c_str(), value_str.length());
        };

        for (auto &bind : controller.inputbinds)
        {
            if (!save_f)
            {
                bind.config.val = static_cast<int32_t>(std::stoi(load_conf(bind.description,
                                                                           std::to_string(bind.config.val))));
                bind.joykey_desc = load_conf(bind.description + "_keydesc",
                                             bind.joykey_desc);
                bind.SDL_port = static_cast<int16_t>(std::stoi(load_conf(bind.description + "_sdl_contr",
                                                                         std::to_string(bind.SDL_port))));
                bind.config.bits.axistrigger = static_cast<int32_t>(std::stoi(load_conf(bind.description + "_anatrig",
                                                                                        std::to_string(bind.config.bits.axistrigger))));
            }
            else
            {
                save_conf(bind.description, std::to_string(bind.config.val));
                save_conf(bind.description + "_anatrig", std::to_string(bind.config.bits.axistrigger));
                save_conf(bind.description + "_sdl_contr", std::to_string(bind.SDL_port));
                save_conf(bind.description + "_keydesc", bind.joykey_desc);
            }
        }
    }
    if (save_f)
        save();
    ini_destroy(ini);
    return true;
}

bool load_inpcfg(retro_input_descriptor *var)
{
    auto lib = CLibretro::get_classinstance();
    lib->use_retropad = false;

    retro_input_descriptor *var2 = var;
    int ports = 0;
    while (var2->description != NULL)
    {
        if (ports < var2->port)
            ports = var2->port;
        var2++;
    }
    ports++;
    lib->core_inpbinds.resize(ports);
    for (auto &controller : lib->core_inpbinds)
        controller.inputbinds.clear();

    while (var->description != NULL)
    {
        coreinput_bind bind;
        bind.description = var->description;
        bind.port = var->port;
        bind.device = var->device;
        auto &settings = bind.config;

        if ((var->device & RETRO_DEVICE_MASK) == RETRO_DEVICE_ANALOG ||
            (var->device & RETRO_DEVICE_MASK) == RETRO_DEVICE_JOYPAD)
        {
            bind.isanalog = (uint8_t)((var->device & RETRO_DEVICE_MASK) == RETRO_DEVICE_ANALOG);
            bind.retro_id = bind.isanalog ? axistocheck(var->id, var->index) : var->id;
            bind.config.bits.axistrigger = 0;
            settings.bits.sdl_id = -1;
            bind.SDL_port = 0;
            settings.bits.joytype = (uint8_t)joytype_::keyboard;
            bind.val = 0;
            bind.joykey_desc = "None";
            lib->core_inpbinds[var->port].inputbinds.push_back(bind);
        }
        var++;
    }

    lib->input_confcrc = 0;
    lib->input_confcrc = MudUtil::crc32(lib->input_confcrc, lib->core_path.c_str(), lib->core_path.size());
    for (auto &controller : lib->core_inpbinds)
        for (auto &bind : controller.inputbinds)
            lib->input_confcrc = MudUtil::crc32(lib->input_confcrc, bind.description.c_str(), bind.description.length());

    return loadinpconf(lib->input_confcrc, false);
}

void close_inpt()
{
    for (auto &bind : Joystick)
    {
        if (bind)
            SDL_GameControllerClose(bind);
    }
    Joystick.clear();
}

void close_inp(int num)
{
    if (Joystick[num])
    {
        SDL_GameControllerClose(Joystick[num]);
        Joystick[num] = NULL;
        Joystick.pop_back();
    }
}

void init_inpt()
{
    Joystick.clear();
    int num = SDL_NumJoysticks();
    if (num)
    {
        for (izrange(i, num))
            Joystick.push_back(SDL_GameControllerOpen(i));
    }
}

void reset_inpt()
{
    for (auto &bind : Joystick)
    {
        if (bind != NULL)
        {
            SDL_GameControllerClose(bind);
            bind = NULL;
        }
    }
    Joystick.clear();
    int num = SDL_NumJoysticks();
    if (num)
    {
        for (izrange(i, num))
            Joystick.push_back(SDL_GameControllerOpen(i));
    }
}

void init_inp(int num)
{
    for (auto &i : Joystick)
    {
        size_t k = &i - &Joystick.front();
        if (k == num)
        {
            if (i)
            {
                SDL_GameControllerClose(i);
                i = SDL_GameControllerOpen(num);
                return;
            }
        }
    }
    Joystick.push_back(SDL_GameControllerOpen(num));
}

struct axisarrdig
{
    const char *name;
    int axis;
};

axisarrdig arr_dig[] = {
    {"lsright", SDL_CONTROLLER_AXIS_LEFTX},
    {"lsleft", SDL_CONTROLLER_AXIS_LEFTX},
    {"lsdown", SDL_CONTROLLER_AXIS_LEFTY},
    {"lsup", SDL_CONTROLLER_AXIS_LEFTY},
    {"rsright", SDL_CONTROLLER_AXIS_RIGHTX},
    {"rsleft", SDL_CONTROLLER_AXIS_RIGHTX},
    {"rsdown", SDL_CONTROLLER_AXIS_RIGHTY},
    {"rsup", SDL_CONTROLLER_AXIS_RIGHTY},
};

void checkbuttons_forui(int selected_inp, bool *isselected_inp, int port)
{
    if (!*isselected_inp)
        return;
    static int framesload = 0;

    if (framesload < 4)
        framesload++;
    else
    {
        auto lib = CLibretro::get_classinstance();
        std::string name;
        auto &bind = lib->core_inpbinds[port].inputbinds[selected_inp];
        const int JOYSTICK_DEAD_ZONE = 0x4000;
        int numkeys = 0;
        const Uint8 *keyboard = SDL_GetKeyboardState(&numkeys);
        for (izrange(i, numkeys))
        {
            if (keyboard[i])
            {
                bind.joykey_desc = SDL_GetScancodeName((SDL_Scancode)i);
                bind.config.bits.sdl_id = i;
                bind.val = 0;
                bind.SDL_port = 0;
                bind.port = port;
                framesload = 0;
                bind.config.bits.joytype = joytype_::keyboard;
                *isselected_inp = false;
                ImGui::SetWindowFocus(NULL);
                return;
            }
        }

        int num = SDL_NumJoysticks();
        SDL_GameControllerUpdate();
        if (num)
        {
            for (izrange(k, num))
            {
                if (checkjs(k))
                {
                    for (izrange(a, SDL_CONTROLLER_AXIS_MAX))
                    {
                        if (bind.isanalog)
                        {
                            Sint16 axis = 0;
                            axis = SDL_GameControllerGetAxis(Joystick[k], (SDL_GameControllerAxis)a);
                            if (abs(axis) < JOYSTICK_DEAD_ZONE)
                                continue;
                            else
                            {
                                name = SDL_GameControllerGetStringForAxis((SDL_GameControllerAxis)a);
                                bind.joykey_desc = name;
                                bind.config.bits.sdl_id = a;
                                bind.val = 0;
                                bind.SDL_port = k;
                                bind.port = port;
                                bind.config.bits.joytype = joytype_::joystick_;
                                *isselected_inp = false;
                                framesload = 0;
                                ImGui::SetWindowFocus(NULL);
                                return;
                            }
                        }
                        else
                        {
                            Sint16 axis = SDL_GameControllerGetAxis(Joystick[k], (SDL_GameControllerAxis)a);
                            if (abs(axis) < JOYSTICK_DEAD_ZONE)
                                continue;
                            else
                            {
                                for (izrange(i, SDL_CONTROLLER_AXIS_MAX + 1))
                                {
                                    if (a == arr_dig[i].axis)
                                    {
                                        bind.joykey_desc = (axis > JOYSTICK_DEAD_ZONE) ? arr_dig[i].name : arr_dig[i + 1].name;
                                        bind.config.bits.sdl_id = a;
                                        bind.val = 0;
                                        bind.SDL_port = k;
                                        bind.port = port;
                                        bind.config.bits.axistrigger = (axis > JOYSTICK_DEAD_ZONE) ? 0x4000 : -0x4000;
                                        bind.config.bits.joytype = joytype_::joystick_;
                                        *isselected_inp = false;
                                        framesload = 0;
                                        ImGui::SetWindowFocus(NULL);
                                        return;
                                    }
                                    if (a == SDL_CONTROLLER_AXIS_TRIGGERLEFT || a == SDL_CONTROLLER_AXIS_TRIGGERRIGHT)
                                    {
                                        if (axis > JOYSTICK_DEAD_ZONE)
                                        {
                                            bind.joykey_desc = (a == SDL_CONTROLLER_AXIS_TRIGGERLEFT) ? "ltrigger" : "rtrigger";
                                            bind.config.bits.axistrigger = 0x4000;
                                            bind.config.bits.sdl_id = a;
                                            bind.val = 0;
                                            bind.SDL_port = k;
                                            bind.port = port;
                                            bind.config.bits.joytype = joytype_::joystick_;
                                            *isselected_inp = false;
                                            framesload = 0;
                                            ImGui::SetWindowFocus(NULL);
                                            return;
                                        }
                                    }
                                }
                            }
                        }
                    }
                    for (izrange(b, SDL_CONTROLLER_BUTTON_MAX))
                    {
                        int btn = SDL_GameControllerGetButton(Joystick[k], (SDL_GameControllerButton)b);
                        if (btn == SDL_PRESSED)
                        {
                            name += SDL_GameControllerGetStringForButton((SDL_GameControllerButton)b);
                            bind.joykey_desc = name;
                            bind.config.bits.sdl_id = b;
                            bind.val = 0;
                            bind.SDL_port = k;
                            bind.port = port;
                            bind.config.bits.joytype = joytype_::button;
                            *isselected_inp = false;
                            framesload = 0;
                            ImGui::SetWindowFocus(NULL);
                            return;
                        }
                    }
                }
            }
        }
    }
}

static int key_pressed(int key)
{
    struct key_map *map = (key_map *)key_map_;
    for (; map->rk != RETROK_LAST; map++)
        if (map->rk == (retro_key)key)
        {
            unsigned sym = SDL_GetScancodeFromKey(map->sym);
            return lr_keymap[sym];
        }
    return false;
}

retro_keyboard_event_t inp_keys = NULL;

void core_kb_callback(retro_keyboard_event_t e)
{
    inp_keys = e;
}

void keys()
{
    lr_keymap = SDL_GetKeyboardState(NULL);
    if (inp_keys != NULL)
    {
        SDL_Keymod mod = SDL_GetModState();
        uint16_t libretro_mod = 0;

        std::tuple<int, int> mask_assoc[] = {
            {KMOD_SHIFT, RETROKMOD_SHIFT},
            {KMOD_CTRL, RETROKMOD_CTRL},
            {KMOD_ALT, RETROKMOD_ALT},
            {KMOD_NUM, RETROKMOD_NUMLOCK},
            {KMOD_CAPS, RETROKMOD_CAPSLOCK},
        };
        for (auto [sdl_mask, libretro_mask] : mask_assoc)
        {
            if (mod & sdl_mask)
                libretro_mod |= libretro_mask;
        }
        struct key_map *map = (key_map *)key_map_;
        for (; map->rk != RETROK_UNKNOWN; map++)
        {
            unsigned sym = SDL_GetScancodeFromKey(map->sym);
            inp_keys(lr_keymap[sym], map->rk, map->sym, libretro_mod);
        }
    }
}

void poll_lr()
{
    auto lib = CLibretro::get_classinstance();

    SDL_GameControllerUpdate();

    keys();
    SDL_PumpEvents();

    memset(&mousiez, 0, sizeof(mousiebind));
    Uint8 btn = SDL_GetRelativeMouseState(&mousiez.rel_x, &mousiez.rel_y);
    SDL_GetMouseState(&mousiez.abs_x, &mousiez.abs_y);
    mousiez.l = (SDL_BUTTON(SDL_BUTTON_LEFT) & btn) ? 1 : 0;
    mousiez.r = (SDL_BUTTON(SDL_BUTTON_RIGHT) & btn) ? 1 : 0;
    mousiez.m = (SDL_BUTTON(SDL_BUTTON_MIDDLE) & btn) ? 1 : 0;
    mousiez.b4 = (SDL_BUTTON(SDL_BUTTON_X1) & btn) ? 1 : 0;
    mousiez.b5 = (SDL_BUTTON(SDL_BUTTON_X2) & btn) ? 1 : 0;

    for (auto &control : lib->core_inpbinds)
    {
        for (auto &bind : control.inputbinds)
        {
            if (bind.config.bits.sdl_id == -1)
            {
                bind.val = 0;
                continue;
            }

            if (bind.config.bits.joytype == joytype_::keyboard)
            {
                bind.val = (lr_keymap[bind.config.bits.sdl_id] == 1) ? 1 : 0;
            }
            else if (bind.config.bits.joytype == joytype_::joystick_)
            {
                if (checkjs(bind.SDL_port))
                {

                    Sint16 axis = SDL_GameControllerGetAxis(Joystick[bind.SDL_port], (SDL_GameControllerAxis)bind.config.bits.sdl_id);

                    if (bind.isanalog)
                        bind.val = axis;
                    else
                    {
                        const int JOYSTICK_DEAD_ZONE = 0x4000;
                        if (abs(axis) < JOYSTICK_DEAD_ZONE)
                            bind.val = 0;
                        else
                        {
                            if (bind.config.bits.axistrigger < 0)
                                bind.val = (axis < -JOYSTICK_DEAD_ZONE) ? 1 : 0;
                            if (bind.config.bits.axistrigger > 0)
                                bind.val = (axis > JOYSTICK_DEAD_ZONE) ? 1 : 0;
                        }
                        continue;
                    }
                }
                else
                    bind.val = 0;
            }
            else if (bind.config.bits.joytype == joytype_::button)
            {
                if (checkjs(bind.SDL_port))
                    bind.val = SDL_GameControllerGetButton(Joystick[bind.SDL_port], (SDL_GameControllerButton)bind.config.bits.sdl_id);
                else
                    bind.val = 0;
            }
        }
    }
}

int16_t input_state(unsigned port, unsigned device, unsigned index,
                    unsigned id)
{
    auto lib = CLibretro::get_classinstance();

    if ((device & RETRO_DEVICE_MASK) == RETRO_DEVICE_MOUSE || (device & RETRO_DEVICE_MASK) == RETRO_DEVICE_LIGHTGUN)
    {
        switch (id)
        {
        case RETRO_DEVICE_ID_MOUSE_X:
            return mousiez.rel_x;
        case RETRO_DEVICE_ID_MOUSE_Y:
            return mousiez.rel_y;
        case RETRO_DEVICE_ID_MOUSE_LEFT:
            return mousiez.l;
        case RETRO_DEVICE_ID_MOUSE_RIGHT:
            return mousiez.r;
        case RETRO_DEVICE_ID_MOUSE_MIDDLE:
            return mousiez.m;
        case RETRO_DEVICE_ID_MOUSE_BUTTON_4:
        case RETRO_DEVICE_ID_LIGHTGUN_TURBO:
            return mousiez.b4;
        case RETRO_DEVICE_ID_MOUSE_BUTTON_5:
        case RETRO_DEVICE_ID_LIGHTGUN_PAUSE:
            return mousiez.b5;
        default:
            return 0;
        }
    }

    if ((device & RETRO_DEVICE_MASK) == RETRO_DEVICE_POINTER)
    {
        vp widthheight = resize_cb();

        const int edge_detect = 32700;
        int scaled_x = -0x8000; /* OOB */
        int scaled_y = -0x8000; /* OOB */
        bool inside = false;
        if (mousiez.abs_x >= 0 && mousiez.abs_x <= widthheight.width)
            scaled_x = ((2 * mousiez.abs_x * 0x7fff) / widthheight.width) - 0x7fff;
        if (mousiez.abs_y >= 0 && mousiez.abs_y <= widthheight.height)
            scaled_y = ((2 * mousiez.abs_y * 0x7fff) / widthheight.height) - 0x7fff;
        mousiez.abs_x -= widthheight.x;
        mousiez.abs_y -= widthheight.y;
        inside = (scaled_x >= -edge_detect) && (scaled_y >= -edge_detect) &&
                 (scaled_x <= edge_detect) && (scaled_y <= edge_detect);
        switch (id)
        {
        case RETRO_DEVICE_ID_POINTER_X:
            return scaled_x;
        case RETRO_DEVICE_ID_POINTER_Y:
            return scaled_y;
        case RETRO_DEVICE_ID_POINTER_PRESSED:
            return (SDL_BUTTON(SDL_BUTTON_LEFT) & mousiez.l);
        case RETRO_DEVICE_ID_LIGHTGUN_IS_OFFSCREEN:
            return !inside;
        }
    }

    if ((device & RETRO_DEVICE_MASK) == RETRO_DEVICE_KEYBOARD)
        return key_pressed(id);

    if ((device & RETRO_DEVICE_MASK) == RETRO_DEVICE_ANALOG || RETRO_DEVICE_JOYPAD)
    {
        if (port > (lib->core_inpbinds.size() - 1))
            return 0;
        for (auto &bind : lib->core_inpbinds[port].inputbinds)
        {
            if (bind.retro_id == (!bind.isanalog) ? id : axistocheck(id, index))
                return bind.val;
            else
                continue;
        }
    }
    return 0;
}
