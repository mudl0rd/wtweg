#include "sdlggerat.h"
#include "imgui.h"
#include "ImGuiFileDialog.h"
#include "ImGuiFileDialogConfig.h"
#include "clibretro.h"
#include <sstream>
#include "utils.h"

ImGuiFileDialog romloader;
const char *ss_filters = "Savestates (*.state){.state}";
const char *filters = "ROMs/ISOs {.sfc,.n64,.v64,.z64,.chd}";
static ImGuiFileDialogFlags flags = ImGuiFileDialogFlags_None;

enum
{
  digi_pad,
  ana_pad,
  mousie
};
int controller_type = digi_pad;

static bool coresettings = false;
const char *checkbox_allowable[] = {"enabled|disabled", "disabled|enabled", "True|False", "False|True", "On|Off", "Off|On"};
const char *true_vals[] = {"enabled", "true", "on"};
bool inputsettings = false;
extern bool closed_dialog = true;

void sdlggerat_menu(CLibretro *instance, std::string *window_str, int * selected_in,bool *isselected_inp)
{
  
  if (ImGui::BeginMainMenuBar())
  {
    if (ImGui::BeginMenu("File"))
    {
      if (ImGui::MenuItem("Load ROM/ISO"))
        romloader.OpenModal("ChooseFileDlgKey", " Choose a ROM/ISO", filters, ".", "", 1, nullptr, flags);

      ImGui::Separator();

      if (ImGui::MenuItem("Load Savestate"))
        romloader.OpenModal("LoadSaveState", "Load a savestate", ss_filters, ".", "", 1, nullptr, flags);

      if (ImGui::MenuItem("Save Savestate"))
        romloader.OpenModal("SaveSaveState", "Save a savestate", ss_filters, ".", "", 1, IGFDUserDatas("SaveFile"), ImGuiFileDialogFlags_ConfirmOverwrite);

      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Options"))
    {
      if (ImGui::MenuItem("Core Settings..."))
        coresettings = true;
      if (ImGui::MenuItem("Input Settings...")) 
        inputsettings = true;
        

      ImGui::Separator();
      if (ImGui::BeginMenu("Input device"))
      {
        if (ImGui::BeginMenu("Joypad"))
        {
          if (ImGui::MenuItem("Analog", nullptr, controller_type == ana_pad))
            controller_type = ana_pad;
          if (ImGui::MenuItem("Digital", nullptr, controller_type == digi_pad))
            controller_type = digi_pad;
          ImGui::EndMenu();
        }
        if (ImGui::MenuItem("Mouse", nullptr, controller_type == mousie))
          controller_type = mousie;
        ImGui::EndMenu();
      }
      ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
  }

  if (romloader.Display("ChooseFileDlgKey"))
  {
    // action if OK
    if (romloader.IsOk())
    {
      std::string filePathName = romloader.GetFilePathName();
      std::string filePath = romloader.GetCurrentPath();
      instance->core_load((char *)filePathName.c_str(), false);
      // action
    }
    // close
    romloader.Close();
  }

  if (romloader.Display("LoadSaveState"))
  {
    // action if OK
    if (romloader.IsOk())
    {
      std::string filePathName = romloader.GetFilePathName();
      std::string filePath = romloader.GetCurrentPath();
      instance->core_savestate(filePathName.c_str(),false);
      // action
    }
    // close
    romloader.Close();
  }

  if (romloader.Display("SaveSaveState"))
  {
    // action if OK
    if (romloader.IsOk())
    {
      std::string filePathName = romloader.GetFilePathName();
      std::string filePath = romloader.GetCurrentPath();
      instance->core_savestate(filePathName.c_str(),true);
    }
    // close
    romloader.Close();
  }

   if(inputsettings && instance->core_isrunning())
   {
     ImGui::PushItemWidth(200);
    ImGui::SetNextWindowSize(ImVec2(550, 660), ImGuiCond_FirstUseEver);
     ImGui::OpenPopup("Input Settings");
  
    if(ImGui::BeginPopupModal("Input Settings",&inputsettings))
    {
      closed_dialog = false;

    std::string str2;
    for (int i=0;i<instance->core_inputbinds.size();i++){
      auto &bind = instance->core_inputbinds[i];
      if(bind.description == "")continue;

       int total_w = 300; ImGui::Text(bind.description.c_str());
       std::string script = "##" + bind.description;
        char* button_str= (char*)bind.joykey_desc.c_str();
       ImGui::SameLine(350);ImGui::SetNextItemWidth(total_w); 
       ImGui::InputText(script.c_str(),button_str, NULL, NULL, NULL);
       if (ImGui::IsItemClicked()) {
      *selected_in = i;
      *isselected_inp=true;
       }

       

      }
       if (ImGui::Button("OK"))
       {
         inputsettings = false;
         *isselected_inp = false;
         closed_dialog = true;
         save_inpcfg();
       }
    ImGui::EndPopup();
      }
   }


if(coresettings && instance->core_isrunning())
{
  ImGui::PushItemWidth(200);
    ImGui::SetNextWindowSize(ImVec2(550, 660), ImGuiCond_FirstUseEver);
    ImGui::OpenPopup("Core Settings");

    if (ImGui::BeginPopupModal("Core Settings",&coresettings))
    {
      for ( auto &bind : instance->core_variables)
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
            
            int total_w = descript.length(); ImGui::Text(descript.c_str()); ImGui::SameLine(450); ImGui::SetNextItemWidth(total_w); 
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
             int total_w = 200; ImGui::Text(descript.c_str()); ImGui::SameLine(650-200); ImGui::SetNextItemWidth(total_w); 
            if (ImGui::BeginCombo(hidden.c_str(), current_item.c_str())) // The second parameter is the label previewed before opening the combo.
            {
              for (int n = 0; n < bind.config_vals.size(); n++)
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