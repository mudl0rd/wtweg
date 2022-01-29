#include "sdlggerat.h"
#include "imgui.h"
#include "ImGuiFileDialog.h"
#include "ImGuiFileDialogConfig.h"
#include "clibretro.h"
#include <sstream>

ImGuiFileDialog romloader;
const char* ss_filters = "Savestates (*.state){.state}";
const char* filters = "Source files (*.cpp *.h *.hpp){.cpp,.h,.hpp},Image files (*.png *.gif *.jpg *.jpeg){.png,.gif,.jpg,.jpeg},.md";
static ImGuiFileDialogFlags flags = ImGuiFileDialogFlags_Default;

enum {digi_pad,ana_pad,mousie};
int controller_type = digi_pad;

static bool coresettings = false;

void sdlggerat_menu()
{
  CLibretro* instance= CLibretro::get_classinstance();
   instance->core_run();
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

            ImGui::Separator();
            if (ImGui::BeginMenu("Input device"))
            {
                if (ImGui::BeginMenu("Joypad"))
				{
                    if(ImGui::MenuItem("Analog", nullptr, controller_type==ana_pad))
                        controller_type = ana_pad;
                     if(ImGui::MenuItem("Digital", nullptr, controller_type==digi_pad))
                        controller_type=digi_pad;
                    ImGui::EndMenu();
				}
                if (ImGui::MenuItem("Mouse", nullptr, controller_type==mousie))
				controller_type = mousie;
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
        }

  if ( romloader.Display("ChooseFileDlgKey")) 
  {
    // action if OK
    if ( romloader.IsOk())
    {
      std::string filePathName =  romloader.GetFilePathName();
      std::string filePath = romloader.GetCurrentPath();
      // action
    }
    // close
     romloader.Close();
  }

  if ( romloader.Display("LoadSaveState")) 
  {
    // action if OK
    if ( romloader.IsOk())
    {
      std::string filePathName =  romloader.GetFilePathName();
      std::string filePath = romloader.GetCurrentPath();
      // action
    }
    // close
     romloader.Close();
  }

    if ( romloader.Display("SaveSaveState")) 
  {
    // action if OK
    if ( romloader.IsOk())
    {
      std::string filePathName =  romloader.GetFilePathName();
      std::string filePath = romloader.GetCurrentPath();
      // action
    }
    // close
     romloader.Close();
  }



  if (coresettings) {
    ImGui::PushItemWidth(200);
     ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Core Settings"))
    {
        
        for (int i=0;i<instance->core_variables.size();i++)
        {
          if(instance->core_variables[i].config_visible)
          {
          std::string descript = instance->core_variables[i].description.c_str();
          std::string var =instance->core_variables[i].var.c_str();
         
          std::vector <std::string> options;
            options.clear();
            char *pch = (char*)instance->core_variables[i].usevars.c_str();
			std::stringstream test(pch);
			std::string segment;
			std::vector<std::string> seglist;
			while (std::getline(test, segment, '|'))
				options.push_back(segment);

        std::string current_item = (char*)options[0].c_str();

ImGui::PushItemWidth(200);
if (ImGui::BeginCombo(descript.c_str(), current_item.c_str())) // The second parameter is the label previewed before opening the combo.
{
    for (int n = 0; n < options.size(); n++)
    {
        bool is_selected = (current_item == options[n].c_str()); // You can store your selection however you want, outside or inside your objects
        if (ImGui::Selectable(options[n].c_str(), is_selected))
            current_item = options[n].c_str();
        if (is_selected)
            ImGui::SetItemDefaultFocus();   // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
    }
    ImGui::EndCombo();
}




          }
        
        }
    

        


      
        
        //click ok when finished adjusting
        if (ImGui::Button("OK finished adjusting")) {
            coresettings = false;
        }

        ImGui::End();
    }
}
}