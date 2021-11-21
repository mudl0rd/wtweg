#include "sdlggerat.h"
#include "imgui.h"
#include "ImGuiFileDialog.h"
#include "ImGuiFileDialogConfig.h"
#include "clibretro.h"

ImGuiFileDialog romloader;
const char* ss_filters = "Savestates (*.state){.state}";
const char* filters = "Source files (*.cpp *.h *.hpp){.cpp,.h,.hpp},Image files (*.png *.gif *.jpg *.jpeg){.png,.gif,.jpg,.jpeg},.md";
static ImGuiFileDialogFlags flags = ImGuiFileDialogFlags_Default;

enum {digi_pad,ana_pad,mousie};
int controller_type = digi_pad;



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
                {}
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
}