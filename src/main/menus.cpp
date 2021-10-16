#include "sdlggerat.h"
#include "imgui.h"

void sdlggerat_menu()
{
  if (ImGui::BeginMainMenuBar())
	    {
		if (ImGui::BeginMenu("File"))
		{
                if (ImGui::MenuItem("Load ROM/ISO"))
                {}
                ImGui::Separator();
                 if (ImGui::MenuItem("Load Savestate"))
                {}
                 if (ImGui::MenuItem("Save Savestate"))
                {}
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
                    if(ImGui::MenuItem("Analog", nullptr, false))
                    { }
                     if(ImGui::MenuItem("Digital", nullptr, true))
                    {}
                    ImGui::EndMenu();
				}
                if (ImGui::MenuItem("Mouse", nullptr, false))
				{}
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
        }
}