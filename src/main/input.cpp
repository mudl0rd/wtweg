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
#include "cJSON.h"
#include "cJSON_Utils.h"




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

struct default_retro
{
    int k;
    int keeb;
} libretro_dmap[] = {
    {RETRO_DEVICE_ID_JOYPAD_B, SDL_SCANCODE_C},
    {RETRO_DEVICE_ID_JOYPAD_Y, SDL_SCANCODE_X},
    {RETRO_DEVICE_ID_JOYPAD_SELECT, SDL_SCANCODE_SPACE},
    {RETRO_DEVICE_ID_JOYPAD_START, SDL_SCANCODE_RETURN},
    {RETRO_DEVICE_ID_JOYPAD_UP, SDL_SCANCODE_UP},
    {RETRO_DEVICE_ID_JOYPAD_DOWN, SDL_SCANCODE_DOWN},
    {RETRO_DEVICE_ID_JOYPAD_LEFT, SDL_SCANCODE_LEFT},
    {RETRO_DEVICE_ID_JOYPAD_RIGHT, SDL_SCANCODE_RIGHT},
    {RETRO_DEVICE_ID_JOYPAD_A, SDL_SCANCODE_D},
    {RETRO_DEVICE_ID_JOYPAD_X, SDL_SCANCODE_S},
    {RETRO_DEVICE_ID_JOYPAD_L, SDL_SCANCODE_A},
    {RETRO_DEVICE_ID_JOYPAD_R, SDL_SCANCODE_Z},
    {RETRO_DEVICE_ID_JOYPAD_L2, SDL_SCANCODE_Q},
    {RETRO_DEVICE_ID_JOYPAD_R2, SDL_SCANCODE_E},
    {RETRO_DEVICE_ID_JOYPAD_L3, -1},
    {RETRO_DEVICE_ID_JOYPAD_R3, -1},
    {joypad_analogx_l, -1},
    {joypad_analogy_l, -1},
    {joypad_analogx_r, -1},
    {joypad_analogy_r, -1},
    {joypad_analog_l2, -1},
    {joypad_analog_r2, -1},
    {joypad_analog_l3, -1},
    {joypad_analog_r3, -1},
};

static std::array <std::string,24>retro_descripts=
{
     "\"RetroPad\" B",
    "\"RetroPad\" Y",
    "\"RetroPad\" Select",
    "\"RetroPad\" Start",
    "\"RetroPad\" Up",
    "\"RetroPad\" Down",
    "\"RetroPad\" Left",
    "\"RetroPad\" Right",
    "\"RetroPad\" A",
    "\"RetroPad\" X",
    "\"RetroPad\" L",
    "\"RetroPad\" R",
    "\"RetroPad\" L2",
    "\"RetroPad\" R2",
    "\"RetroPad\" L3",
    "\"RetroPad\" R3",
    "\"RetroPad\" Left Stick X Axis",
    "\"RetroPad\" Left Stick Y Axis",
    "\"RetroPad\" Right Stick X Axis",
    "\"RetroPad\" Right Stick Y Axis",
    "\"RetroPad\" Analog L2",
    "\"RetroPad\" Analog R2",
    "\"RetroPad\" Analog L3",
    "\"RetroPad\" Analog R3"
};

void CLibretro::reset_retropad()
{

    core_inpbinds.clear();
    core_inpbinds.resize(2);
    core_inputttypes.clear();

    inp_keys = NULL;

    for (int i = 0; i < 2; i++)
    {
        // Assume "RetroPad"....fuck me

        std::vector<coreinput_bind> bind2;
        bind2.clear();

        for (auto &retro_descript : retro_descripts)
        {
            size_t j = &retro_descript - &retro_descripts.front();
            coreinput_bind bind;
            bind.device = (j < 15) ? RETRO_DEVICE_JOYPAD : RETRO_DEVICE_ANALOG;
            bind.isanalog = (j > 15);
            bind.retro_id = j;
            bind.config.bits.axistrigger = 0;
            bind.config.bits.sdl_id = (i == 0 && j < 15) ? libretro_dmap[j].keeb : -1;
            bind.config.bits.joytype = (uint8_t)joytype_::keyboard;
            bind.val = 0;
            bind.SDL_port = 0;
            bind.port = i;
            bind.description = retro_descript;
            bind.joykey_desc = (i == 0 && j < 15) ? SDL_GetScancodeName((SDL_Scancode)libretro_dmap[j].keeb) : "None";
            bind2.push_back(bind);
        }
        core_inpbinds[i].inputbinds = bind2;
        core_inpbinds[i].controller_type = RETRO_DEVICE_JOYPAD;
    }
}

int  CLibretro::axistocheck(int id, int index)
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

bool CLibretro::checkjs(int port)
{
    for (auto &i : Joystick)
    {
        size_t k = &i - &Joystick.front();
        if (k == port)
        {
            return SDL_GameControllerGetAttached(i);
        }
    }
    return false;
}

bool CLibretro::loadcontconfig(bool save_f)
{

    if (!save_f)
    {
        for (auto &i : core_inpbinds)
        {
            size_t k = &i - &core_inpbinds.front();
            for (auto &inp_cfg : core_inputttypes)
            {
                size_t l = &inp_cfg - &core_inputttypes.front();
                if (k == l)
                {
                    i.controlinfo = inp_cfg;
                    for (auto &j : inp_cfg)
                    {
                        if ((j.id & RETRO_DEVICE_MASK) == RETRO_DEVICE_JOYPAD)
                        {
                            i.controller_type = j.id;
                            break;
                        }
                    }
                }
            }
        }
    }

    unsigned sz_coreconfig = MudUtil::get_filesize(core_config.c_str());
    std::vector<uint8_t> data;
    int portage = 0;
    cJSON *ini = NULL;
    if (sz_coreconfig)
    {
        data = MudUtil::load_data(core_config.c_str());
        ini = cJSON_Parse((char *)data.data());
    }
    else
        ini = cJSON_CreateObject();

    cJSON *config = NULL;
    cJSON *config_entries = NULL;
    if (cJSON_HasObjectItem(ini, std::to_string(config_crc).c_str()))
    {
        config = cJSON_GetObjectItemCaseSensitive(ini, std::to_string(config_crc).c_str());
        config_entries = cJSON_GetArrayItem(config, 0);
    }

    else
    {
        save_f = true;
        config = cJSON_AddArrayToObject(ini, std::to_string(config_crc).c_str());
        config_entries = cJSON_CreateObject();
        cJSON_AddItemToArray(config, config_entries);
    }

    for (auto &controller : core_inpbinds)
    {
        std::string play = std::to_string(portage) + "_controllerport";
        if (cJSON_HasObjectItem(config_entries, play.c_str()))
        {
            cJSON *configval = cJSON_GetObjectItemCaseSensitive(config_entries, play.c_str());
            if (save_f)
                cJSON_SetNumberValue(configval, controller.controller_type);
            else
                controller.controller_type = configval->valueint;
        }
        else
        {
            cJSON_AddNumberToObject(config_entries, play.c_str(), controller.controller_type);
            save_f = true;
        }
        portage++;
    }
    if (save_f)
    {
        std::string json = cJSON_Print(ini);
        MudUtil::save_data((unsigned char *)json.c_str(), json.length(), core_config.c_str());
    }
    cJSON_Delete(ini);
    return true;
}

bool CLibretro::loadinpconf(uint32_t checksum, bool save_f)
{
    unsigned sz_coreconfig = MudUtil::get_filesize(core_config.c_str());
    std::vector<uint8_t> data = MudUtil::load_data((const char *)core_config.c_str());
    int portage = 0;

    cJSON *inputbinds = NULL;
    cJSON *ini = NULL;
    bool upd = false;
    if (!sz_coreconfig)
    {
        ini = cJSON_CreateObject();
        save_f = true;
        inputbinds = cJSON_AddArrayToObject(ini, std::to_string(checksum).c_str());
    }
    else
    {
        ini = cJSON_Parse((char *)data.data());
        if (cJSON_HasObjectItem(ini, std::to_string(checksum).c_str()))
        {
            upd = save_f;
            inputbinds = cJSON_GetObjectItemCaseSensitive(ini, std::to_string(checksum).c_str());
        }

        else
        {
            save_f = true;
            inputbinds = cJSON_AddArrayToObject(ini, std::to_string(checksum).c_str());
        }
    }

    for (auto &controller : core_inpbinds)
    {
        size_t i = &controller - &core_inpbinds.front();
        std::string play = std::to_string(i) + "_binds";
        cJSON *binds_entries = NULL;
        cJSON *binds = NULL;
        cJSON *binds_str = cJSON_GetArrayItem(inputbinds, i);
        if (binds_str)
        {
            binds = cJSON_GetObjectItemCaseSensitive(binds_str, play.c_str());
            binds_entries = cJSON_GetArrayItem(binds, 0);
        }
        else
        {
            cJSON *player_obj = cJSON_CreateObject();
            cJSON_AddItemToArray(inputbinds, player_obj);
            play = std::to_string(i) + "_binds";
            binds = cJSON_AddArrayToObject(player_obj, play.c_str());
            binds_entries = cJSON_CreateObject();
            cJSON_AddItemToArray(binds, binds_entries);
        }
        for (auto &bind : controller.inputbinds)
        {
            std::string bindstring;
            auto entryupd = [&save_f](std::string entry, cJSON *entryjs, auto &bind)
            {
                if (cJSON_HasObjectItem(entryjs, entry.c_str()))
                {
                    cJSON *configval = cJSON_GetObjectItemCaseSensitive(entryjs, entry.c_str());
                    if (save_f)
                        cJSON_SetNumberValue(configval, bind);
                    else
                        bind = cJSON_GetNumberValue(configval);
                }
                else
                {
                    cJSON_AddNumberToObject(entryjs, entry.c_str(), bind);
                    save_f = true;
                }
            };
            entryupd(bind.description, binds_entries, bind.config.val);
            bindstring = bind.description + "_anatrig";
            entryupd(bindstring, binds_entries, bind.config.bits.axistrigger);
            bindstring = bind.description + "_sdlport";
            entryupd(bindstring, binds_entries, bind.SDL_port);
            bindstring = bind.description + "_sdlid";
            entryupd(bindstring, binds_entries, bind.config.bits.sdl_id);
            bindstring = bind.description + "_joytype";
            entryupd(bindstring, binds_entries, bind.config.bits.joytype);
            bindstring = bind.description + "_keydesc";
            if (cJSON_HasObjectItem(binds_entries, bindstring.c_str()))
            {
                cJSON *configval = cJSON_GetObjectItemCaseSensitive(binds_entries, bindstring.c_str());
                if (save_f)
                    cJSON_SetValuestring(configval, bind.joykey_desc.c_str());
                else
                    bind.joykey_desc = cJSON_GetStringValue(configval);
            }
            else
            {
                cJSON_AddStringToObject(binds_entries, bindstring.c_str(), bind.joykey_desc.c_str());
                save_f = true;
            }
        }
    }
    if (save_f)
    {
        std::string json = cJSON_Print(ini);
        MudUtil::save_data((unsigned char *)json.c_str(), json.length(), core_config.c_str());
    }
    cJSON_Delete(ini);

    return true;
}

bool CLibretro::load_inpcfg(retro_input_descriptor *var)
{
    use_retropad = false;

    retro_input_descriptor *var2 = var;
    int ports = 0;
    while (var2->description != NULL)
    {
        if (ports < var2->port)
            ports = var2->port;
        var2++;
    }
    ports++;
    core_inpbinds.resize(ports);
    for (auto &controller : core_inpbinds)
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
            core_inpbinds[var->port].inputbinds.push_back(bind);
        }
        var++;
    }

    input_confcrc = config_crc;
    for (auto &controller : core_inpbinds)
        for (auto &bind : controller.inputbinds)
            input_confcrc = MudUtil::crc32(input_confcrc, bind.description.c_str(), bind.description.length());

    return loadinpconf(input_confcrc, false);
}

void CLibretro::close_inpt()
{
    for (auto &bind : Joystick)
    {
        if (bind)
            SDL_GameControllerClose(bind);
    }
    Joystick.clear();
}

void CLibretro::close_inp(int num)
{
    for (auto &i : Joystick)
    {
        size_t k = &i - &Joystick.front();
        if (k == num)
        {
            SDL_GameControllerClose(i);
            i = NULL;
            Joystick.pop_back();
            return;
        }
    }
}

void CLibretro::init_inpt()
{
    Joystick.clear();
    int num = SDL_NumJoysticks();
    if (num)
    {
        for (izrange(i, num))
            Joystick.push_back(SDL_GameControllerOpen(i));
    }
}

void CLibretro::init_inp(int num)
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

void CLibretro::checkbuttons_forui(int selected_inp, bool *isselected_inp, int port)
{
    if (!*isselected_inp)
        return;
    static int framesload = 0;

    if (framesload < 4)
        framesload++;
    else
    {
        std::string name;
        auto &bind = core_inpbinds[port].inputbinds[selected_inp];
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

int CLibretro::key_pressed(int key)
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


void CLibretro::keys()
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

void CLibretro::poll_lr()
{
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

    for (auto &control : core_inpbinds)
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

int16_t CLibretro::input_state(unsigned port, unsigned device, unsigned index,
                    unsigned id)
{
    if ((device & RETRO_DEVICE_MASK) == RETRO_DEVICE_MOUSE)
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
            return mousiez.b4;
        case RETRO_DEVICE_ID_MOUSE_BUTTON_5:
            return mousiez.b5;
        default:
            return 0;
        }
    }

    if ((device & RETRO_DEVICE_MASK) == RETRO_DEVICE_POINTER || (device & RETRO_DEVICE_MASK) == RETRO_DEVICE_LIGHTGUN)
    {
        vp widthheight = video.resize_cb();

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

        if(index>0)return 0;
        switch (id)
        {
        case RETRO_DEVICE_ID_POINTER_X:
        case RETRO_DEVICE_ID_LIGHTGUN_SCREEN_X:
            return scaled_x;
        case RETRO_DEVICE_ID_POINTER_Y:
        case RETRO_DEVICE_ID_LIGHTGUN_SCREEN_Y:
            return scaled_y;
        case RETRO_DEVICE_ID_POINTER_PRESSED:
            return mousiez.l;
        case RETRO_DEVICE_ID_LIGHTGUN_IS_OFFSCREEN:
            return !inside;
        case RETRO_DEVICE_ID_LIGHTGUN_AUX_B:
            return mousiez.b4;
        case RETRO_DEVICE_ID_LIGHTGUN_START:
            return mousiez.b5;
        }
    }

    if ((device & RETRO_DEVICE_MASK) == RETRO_DEVICE_KEYBOARD)
        return key_pressed(id);

    if ((device & RETRO_DEVICE_MASK) == RETRO_DEVICE_ANALOG || RETRO_DEVICE_JOYPAD)
    {
        for (auto &binds : core_inpbinds)
        {
            size_t k = &binds - &core_inpbinds.front();
            if (k == port)
            {
                for (auto &bind : binds.inputbinds)
                {
                    if ((device & RETRO_DEVICE_MASK) == RETRO_DEVICE_JOYPAD)
                    {
                        if (bind.retro_id == id)
                            return bind.val;
                    }
                    else
                    {
                        if (bind.isanalog == true && bind.retro_id == axistocheck(id, index))
                            return bind.val;
                    }
                }
            }
        }
    }
    return 0;
}
