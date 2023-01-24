#define IMGUI_DISABLE_DEMO_WINDOWS
#include "imgui.h"
#include "ImGuiFileDialog.h"
#include "ImGuiFileDialogConfig.h"

#include <sstream>
#include "utils.h"
#include "io.h"
#include "clibretro.h"

#ifndef _WIN32
#define stricmp strcasecmp
#endif
const char *ss_filters = "Savestates (*.state){.state}";
static ImGuiFileDialogFlags flags = ImGuiFileDialogFlags_CaseInsensitiveExtention;

enum
{
  digi_pad,
  ana_pad,
  mousie,
  input_keyboard,
  lightgun,
  pointer
};
int controller_type = digi_pad;

const char *checkbox_allowable[] = {"enabled|disabled", "disabled|enabled", "True|False", "False|True", "On|Off", "Off|On"};
const char *true_vals[] = {"enabled", "true", "on"};

static bool coreselect = false;
bool pergame_ = false;
static std::string filenamepath;



static auto vector_getter = [](void *data, int n, const char **out_text)
{
  const std::vector<core_info> *v = (std::vector<core_info> *)data;
  *out_text = v->at(n).core_name.c_str();
  return true;
};

bool loadfile(CLibretro *instance, const char *file, const char *core_file, bool pergame)
{
  if (core_file != NULL)
  {
    instance->core_load((char *)file, pergame, (char *)core_file, false);
    return false;
  }
  else
  {
    pergame_ = pergame;
    filenamepath = file;
    coreselect = true;
    return true;
  }
}

void popup_widget(bool *flag, const char *title, const char *msg)
{
  ImGui::OpenPopup(title);
  ImGuiIO &io = ImGui::GetIO();
  ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
  if (ImGui::BeginPopupModal(title, NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse))
  {
    ImGui::Text(msg);
    ImGui::Separator();
    if (ImGui::Button("OK", ImVec2(120, 0)))
    {
      ImGui::CloseCurrentPopup();
      *flag = false;
    }
    ImGui::EndPopup();
  }
}

void sdlggerat_menu(CLibretro *instance, std::string *window_str)
{

  static bool inputsettings = false;
  static bool coresettings = false;
  static bool aboutbox = false;
  static bool load_core = false;
  static bool no_cores = false;
  static int selected_inp = 0;
  static bool isselected_inp = false;
  static int selected_port = 0;
  ImGuiIO &io = ImGui::GetIO();

  if (ImGui::BeginMainMenuBar())
  {
    if (ImGui::BeginMenu("File"))
    {
      if (ImGui::MenuItem("Load ROM/ISO"))
      {
        if (instance->coreexts == "")
        {
          no_cores = true;
        }
        else
        
          ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", " Choose a ROM/ISO", instance->coreexts.c_str(), ".", "", 1, nullptr, flags);
      }

      if (ImGui::MenuItem("Load contentless libretro core"))
        load_core = true;

      if (instance->core_isrunning())
      {

        ImGui::Separator();
        if (ImGui::BeginMenu("Quick save slot"))
        {
          for (int i = 0; i < 9; i++)
          {
            std::string player = "Quick save slot " + std::to_string(i + 1);
            if (ImGui::MenuItem(player.c_str(), nullptr, instance->save_slot == i))
              instance->save_slot = i;
          }
          ImGui::EndMenu();
        }

        if (ImGui::MenuItem("Load Savestate"))
        {
          if (instance->core_isrunning())
            ImGuiFileDialog::Instance()->OpenDialog("LoadSaveState", "Load a savestate", ".state", ".", "", 1, nullptr, flags);
        }

        if (ImGui::MenuItem("Save Savestate"))
        {
          if (instance->core_isrunning())
            ImGuiFileDialog::Instance()->OpenDialog("SaveSaveState", "Save a savestate", ".state", ".", "", 1, IGFDUserDatas("SaveFile"), ImGuiFileDialogFlags_ConfirmOverwrite);
        }
      }

      ImGui::EndMenu();
    }
    if (instance->core_isrunning())
    {
      if (ImGui::BeginMenu("Options"))
      {
        if (ImGui::MenuItem("Core Settings..."))
          coresettings = true;
        if (ImGui::MenuItem("Input Settings..."))
          inputsettings = true;

        if (instance->core_inputdesc[0].size() > 1)
        {
          ImGui::Separator();

          for (int i = 0; i < 2; i++)
          {
            std::string player = "Player " + std::to_string(i + 1);
            if (ImGui::BeginMenu(player.c_str()))
            {
              for (int j = 0; j < instance->core_inputdesc[i].size(); j++)
              {
                const char *label = instance->core_inputdesc[i][j].desc.c_str();
                if (ImGui::MenuItem(label, nullptr,
                                    instance->controller_type[i] == instance->core_inputdesc[i][j].id))
                {
                  instance->controller_type[i] = instance->core_inputdesc[i][j].id;
                  instance->core_changinpt(instance->controller_type[i], i);
                }
              }
              ImGui::EndMenu();
            }
          }
        }

        ImGui::EndMenu();
      }
    }

    ImGui::EndMainMenuBar();
  }

  ImVec2 maxSizedlg = ImVec2((float)io.DisplaySize.x * 0.7f, (float)io.DisplaySize.y * 0.7f);
  ImVec2 minSizedlg = ImVec2((float)io.DisplaySize.x * 0.4f, (float)io.DisplaySize.y * 0.4f);
  if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey",32, minSizedlg, maxSizedlg))
  {
    // action if OK
    if (ImGuiFileDialog::Instance()->IsOk())
    {
      std::string filePathName =  ImGuiFileDialog::Instance()->GetFilePathName();
      std::string filePath =  ImGuiFileDialog::Instance()->GetCurrentPath();
      coreselect = loadfile(instance, (char *)filePathName.c_str(), NULL, false);
      filenamepath = filePathName;
    }

     ImGuiFileDialog::Instance()->Close();
  }

  if (no_cores)
  {
    popup_widget(&no_cores, "No libretro cores", "There is no ROM/ISO loading cores detected.");
    return;
  }

  if (coreselect)
  {
    int hits = 0;
    int selected_core = 0;
    int iter = 0;
    std::vector<core_info> cores_info;
    cores_info.clear();
    bool found = false;

    for (auto &core : instance->cores)
    {

      if (core.no_roms)
        continue;

      std::string core_ext = core.core_extensions;
      std::string ext = filenamepath;
      ext = ext.substr(ext.find_last_of(".") + 1);
      std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
      if (core_ext.find(ext) != std::string::npos)
      {
        hits++;
        selected_core = iter;
        cores_info.push_back(core);
        found = true;
      }
      iter++;
    }
    if (hits == 1 && found)
    {
      instance->core_load((char *)filenamepath.c_str(), pergame_, (char *)cores_info.at(0).core_path.c_str(), false);
      coreselect = false;
      return;
    }
    if (!found)
    {
      popup_widget(&coreselect, "Core Load Error", "There is no core to load this particular bit of content.");
      coreselect = false;
      return;
    }
    else
      ImGui::OpenPopup("Select a core");

    ImGui::SetNextWindowSizeConstraints(ImVec2(io.DisplaySize.x * 0.3f, io.DisplaySize.y * 0.3f),
    ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f));
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Select a core", &coreselect, ImGuiWindowFlags_AlwaysAutoResize))
    {
      static int listbox_item_current = 0;
      ImGui::ListBox("Select a core",
                     &listbox_item_current, vector_getter, static_cast<void *>(&cores_info), cores_info.size());
      if (ImGui::Button("OK"))
      {
        instance->core_load((char *)filenamepath.c_str(), pergame_, (char *)cores_info.at(listbox_item_current).core_path.c_str(), false);
        coreselect = false;
      }
      ImGui::Bullet();
      ImGui::SameLine();
      ImGui::TextWrapped("WTFweg couldn't determine the core to use.");
      ImGui::Bullet();
      ImGui::SameLine();
      ImGui::TextWrapped("Choose the specific core to load the ROM/ISO.");
      ImGui::EndPopup();
    }
  }
 
  if (ImGuiFileDialog::Instance()->Display("LoadSaveState",32,minSizedlg, maxSizedlg))
  {
    // action if OK
    if ( ImGuiFileDialog::Instance()->IsOk())
    {
      std::string filePathName =  ImGuiFileDialog::Instance()->GetFilePathName();
      instance->core_savestate(filePathName.c_str(), false);
      // action
    }
    // close
     ImGuiFileDialog::Instance()->Close();
  }

  if (ImGuiFileDialog::Instance()->Display("SaveSaveState", 32,minSizedlg, maxSizedlg))
  {
    // action if OK
    if (ImGuiFileDialog::Instance()->IsOk())
    {
      std::string filePathName =  ImGuiFileDialog::Instance()->GetFilePathName();
      instance->core_savestate(filePathName.c_str(), true);
    }
    // close
     ImGuiFileDialog::Instance()->Close();
  }

  if (load_core)
  {
    std::vector<core_info> cores_info;
    cores_info.clear();
    bool found = false;
    for (auto &core : instance->cores)
      if (core.no_roms)
      {
        cores_info.push_back(core);
        found = true;
      }

    if (!found)
    {
      popup_widget(&load_core, "No contentless core", "There is no contentless cores detected.");
      return;
    }

    ImGui::SetNextWindowSizeConstraints(ImVec2(io.DisplaySize.x * 0.3f, io.DisplaySize.y * 0.3f),
    ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f));
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::OpenPopup("Select a contentless core to load");
    if (ImGui::BeginPopupModal("Select a contentless core to load", &load_core, ImGuiWindowFlags_AlwaysAutoResize))
    {
      static int listbox_item_current = 0;
      ImGui::ListBox("Select a contentless core",
                     &listbox_item_current, vector_getter, static_cast<void *>(&cores_info), cores_info.size());
      if (ImGui::Button("OK"))
      {
        instance->core_load(NULL, false, (char *)cores_info.at(listbox_item_current).core_path.c_str(), true);
        load_core = false;
      }
      ImGui::Bullet();
      ImGui::SameLine();
      ImGui::TextWrapped("Choose the specific core to load.");
      ImGui::Bullet();
      ImGui::SameLine();
      ImGui::TextWrapped("These are ones that load their own assets.");
      ImGui::EndPopup();
    }
  }

  

  if (inputsettings)
  {
    checkbuttons_forui(selected_inp, &isselected_inp, selected_port);
    if (!instance->core_inputbinds[0].size())
    {
      popup_widget(&inputsettings, "No input settings", "There is no input settings for this particular core.");
      return;
    }

    

    ImGui::OpenPopup("Input Settings");
    ImGui::SetNextWindowSizeConstraints(ImVec2(io.DisplaySize.x * 0.7f, io.DisplaySize.y * 0.3f),
    ImVec2(io.DisplaySize.x * 0.7f, io.DisplaySize.y * 0.5f));
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("Input Settings", &inputsettings, ImGuiWindowFlags_AlwaysAutoResize))
    {
      if (!instance->core_inputbinds[1].size())
      {
        int descnum = 1;
        for (size_t i = 0; i < instance->core_inputbinds[0].size(); i++)
        {
          auto &bind = instance->core_inputbinds[0][i];
          if (bind.description == "")
            continue;

          ImGui::TextWrapped("%s", bind.description.c_str());
          std::string script = "##" + bind.description;
          char *button_str = (char *)bind.joykey_desc.c_str();
          ImVec2 sz = ImGui::GetWindowSize();
          ImGui::SameLine(sz.x * 0.78);
          ImGui::SetNextItemWidth(sz.x * 0.2);
          ImGui::InputText(script.c_str(), button_str, 0, 0, NULL);
          if (ImGui::IsItemClicked())
          {
            selected_inp = i;
            isselected_inp = true;
            selected_port = descnum - 1;
          }
        }
      }
      else if (ImGui::BeginTabBar("MyTabBar", ImGuiTabBarFlags_None))
      {
        int descnum = 1;
        for (auto &controller : instance->core_inputbinds)
        {
          std::string descstring = "Player " + std::to_string(descnum);
          if (ImGui::BeginTabItem(descstring.c_str()))
          {
            for (size_t i = 0; i < controller.size(); i++)
            {
              auto &bind = controller[i];
              if (bind.description == "")
                continue;

              ImGui::TextWrapped("%s", bind.description.c_str());
              std::string script = "##" + bind.description;
              char *button_str = (char *)bind.joykey_desc.c_str();
              ImVec2 sz = ImGui::GetWindowSize();
              ImGui::SameLine(sz.x * 0.80);
              ImGui::SetNextItemWidth(sz.x * 0.2);
              ImGui::InputText(script.c_str(), button_str, 0, 0, NULL);
              if (ImGui::IsItemClicked())
              {
                selected_inp = i;
                isselected_inp = true;
                selected_port = descnum - 1;
              }
            }

            ImGui::EndTabItem();
          }
          descnum++;
        }
        ImGui::EndTabBar();
      }
      if (ImGui::Button("OK"))
      {
        inputsettings = false;
        isselected_inp = false;
        save_inpcfg();
      }
      ImGui::EndPopup();
    }
  }

  if (coresettings)
  {

    if (!instance->core_variables.size())
    {
      popup_widget(&coresettings, "No core settings", "There is no core settings for this particular core.");
      return;
    }

    ImGui::OpenPopup("Core Settings");

    ImGui::SetNextWindowSizeConstraints(ImVec2(io.DisplaySize.x * 0.7f, io.DisplaySize.y * 0.1f),
    ImVec2(io.DisplaySize.x * 0.7f, io.DisplaySize.y * 0.5f));
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("Core Settings", &coresettings, ImGuiWindowFlags_AlwaysAutoResize))
    {
      for (auto &bind : instance->core_variables)
      {

        if (bind.config_visible)
        {
          std::string descript = bind.description;
          std::string usedv = bind.usevars;
          std::string var = bind.var;

          int sel_idx = bind.sel_idx;
          std::string current_item = bind.config_vals[sel_idx];
          bool checkbox_made = false;
          bool checkbox_enabled = false;
          for (int j = 0; j < IM_ARRAYSIZE(checkbox_allowable); j++)
          {
            if (checkbox_enabled)
              break;
            if (stricmp(usedv.c_str(), checkbox_allowable[j]) == 0)
            {
              checkbox_made = true;
              for (int k = 0; k < IM_ARRAYSIZE(true_vals); k++)
                if (stricmp(var.c_str(), true_vals[k]) == 0)
                  checkbox_enabled = true;
            }
          }

          std::string hidden = "##" + descript;
          if (checkbox_made)
          {

            int total_w = descript.length();
            ImGui::TextWrapped("%s", descript.c_str());
            float w = ImGui::CalcItemWidth();
            ImVec2 sz = ImGui::GetWindowSize();
            ImGui::SameLine(sz.x * 0.65);
            ImGui::SetNextItemWidth(total_w);
            if (ImGui::Checkbox(hidden.c_str(), &checkbox_enabled))
            {
              bind.sel_idx ^= 1;
              std::string change = bind.config_vals[bind.sel_idx];
              bind.var = change;
              instance->variables_changed = true;
            }
          }
          else
          {

            ImGui::TextWrapped("%s", descript.c_str());
            ImVec2 sz = ImGui::GetWindowSize();
            ImGui::SameLine(sz.x * 0.65);
            ImGui::SetNextItemWidth(sz.x * 0.3);
            if (ImGui::BeginCombo(hidden.c_str(), current_item.c_str())) // The second parameter is the label previewed before opening the combo.
            {
              for (size_t n = 0; n < bind.config_vals.size(); n++)
              {
                bool is_selected = (bind.sel_idx == n); // You can store your selection however you want, outside or inside your objects
                if (ImGui::Selectable(bind.config_vals[n].c_str(), is_selected))
                {
                  bind.sel_idx = n;
                  std::string change = bind.config_vals[bind.sel_idx];
                  bind.var = change;
                  instance->variables_changed = true;
                }

                if (is_selected)
                  ImGui::SetItemDefaultFocus();
              }
              ImGui::EndCombo();
            }
          }
        }
      }

      // click ok when finished adjusting
      if (ImGui::Button("OK"))
      {
        coresettings = false;
        instance->save_coresettings();
      }

      ImGui::EndPopup();
    }
  }
}