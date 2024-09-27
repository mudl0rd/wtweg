#define IMGUI_DISABLE_DEMO_WINDOWS
#include "imgui.h"
#include "ImGuiFileDialog.h"
#include "ImGuiFileDialogConfig.h"

#include <sstream>
#include "mudutils/utils.h"
#include "inout.h"
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
const char *checkbox_allowablev1[] = {"enabled|disabled", "disabled|enabled", "True|False", "False|True", "On|Off", "Off|On"};
const char *checkbox_allowable[] = {"enabled", "disabled", "True", "False", "On", "Off"};
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

struct offsets
{
  ImU32 col;
};
offsets colors[4] = {IM_COL32(0, 255, 0, 255), IM_COL32(0, 255, 0, 255), IM_COL32(255, 255, 0, 255),
                     IM_COL32(255, 0, 0, 255)};

struct ExampleAppLog
{
  ImGuiTextBuffer Buf;
  ImVector<int> LineOffsets; // Index to lines offset. We maintain this with AddLog() calls.
  ImVector<ImU32> LineCol;
  bool AutoScroll; // Keep scrolling if already at the bottom.

  ExampleAppLog()
  {
    AutoScroll = true;
    Clear();
  }

  void Clear()
  {
    Buf.clear();
    LineOffsets.clear();
    LineOffsets.push_back(0);
    LineCol.clear();
    LineCol.push_back(0);
  }

  void AddLog(enum retro_log_level level, const char *fmt, ...) IM_FMTARGS(3)
  {
    int old_size = Buf.size();
    va_list args;
    va_start(args, fmt);
    Buf.appendfv(fmt, args);
    va_end(args);
    LineCol.push_back(colors[level].col);
    LineOffsets.push_back(old_size);
  }

  void Draw(const char *title, bool *p_open = NULL)
  {
    ImGuiIO &io = ImGui::GetIO();
    ImGui::SetNextWindowSizeConstraints(ImVec2(io.DisplaySize.x * 0.3f, io.DisplaySize.y * 0.3f),
                                        ImVec2(io.DisplaySize.x * 0.6f, io.DisplaySize.y * 0.6f));
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.2f, io.DisplaySize.y * 0.5f), ImGuiCond_Once, ImVec2(0.5f, 0.5f));
    if (!ImGui::Begin(title, p_open, ImGuiWindowFlags_NoCollapse))
    {
      ImGui::End();
      return;
    }

    // Main window
    ImGui::SameLine();
    bool clear = ImGui::Button("Clear");
    ImGui::SameLine();
    bool copy = ImGui::Button("Copy");
    ImGui::Separator();
    ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

    if (clear)
      Clear();
    if (copy)
      ImGui::LogToClipboard();

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
    const char *buf = Buf.begin();
    const char *buf_end = Buf.end();
    {
      // The simplest and easy way to display the entire buffer:
      //   ImGui::TextUnformatted(buf_begin, buf_end);
      // And it'll just work. TextUnformatted() has specialization for large blob of text and will fast-forward
      // to skip non-visible lines. Here we instead demonstrate using the clipper to only process lines that are
      // within the visible area.
      // If you have tens of thousands of items and their processing cost is non-negligible, coarse clipping them
      // on your side is recommended. Using ImGuiListClipper requires
      // - A) random access into your data
      // - B) items all being the  same height,
      // both of which we can handle since we an array pointing to the beginning of each line of text.
      // When using the filter (in the block of code above) we don't have random access into the data to display
      // anymore, which is why we don't use the clipper. Storing or skimming through the search result would make
      // it possible (and would be recommended if you want to search through tens of thousands of entries).
      ImGuiListClipper clipper;
      clipper.Begin(LineOffsets.Size);
      while (clipper.Step())
      {
        for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++)
        {
          const char *line_start = buf + LineOffsets[line_no];
          const char *line_end = (line_no + 1 < LineOffsets.Size) ? (buf + LineOffsets[line_no + 1] - 1) : buf_end;
          ImGui::PushStyleColor(ImGuiCol_Text, LineCol[line_no]);
          ImGui::TextUnformatted(line_start, line_end);
          ImGui::PopStyleColor();
        }
      }
      clipper.End();
    }
    ImGui::PopStyleVar();

    if (AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
      ImGui::SetScrollHereY(1.0f);

    ImGui::EndChild();
    ImGui::End();
  }
};
static ExampleAppLog my_log;
// Usage:
//  static ExampleAppLog my_log;
//  my_log.AddLog("Hello %d world\n", 123);
//  my_log.Draw("title");

void add_log(enum retro_log_level level, const char *fmt)
{
  my_log.AddLog(level, fmt);
}

static void HelpMarker(const char *desc)
{
  ImGui::TextDisabled("(?)");
  if (ImGui::IsItemHovered())
  {
    ImGui::BeginTooltip();
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
    ImGui::TextUnformatted(desc);
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
  }
}

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
  static bool open_log = false;
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
          for (izrange(i, 9))
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

    if (!instance->core_isrunning())
    {
      if (ImGui::BeginMenu("Options"))
      {
        if (instance->use_retropad)
          if (ImGui::MenuItem("Input Settings (Default)"))
            inputsettings = true;

        ImGui::EndMenu();
      }
    }
    else if (instance->core_isrunning())
    {
      if (ImGui::BeginMenu("Options"))
      {
        if (ImGui::MenuItem("Core Settings..."))
          coresettings = true;
        if (!instance->use_retropad)
        {
          if (ImGui::MenuItem("Input Settings (Core-specific)..."))
            inputsettings = true;
        }
        else
        {
          if (ImGui::MenuItem("Input Settings (Default)..."))
            inputsettings = true;
        }

        if (ImGui::MenuItem("Core log", nullptr,
                            open_log == true))
          open_log = !open_log;

        if (instance->core_inpbinds.size())
        {

          for (auto &i : instance->core_inpbinds)
          {
            size_t k = &i - &instance->core_inpbinds.front();
            if (!k && i.controlinfo.size() >= 1)
            {
              ImGui::Separator();
            }
            if (i.controlinfo.size() >= 1)
            {
              std::string player = "Player " + std::to_string(k + 1);

              if (ImGui::BeginMenu(player.c_str()))
              {
                for (auto &inp2 : i.controlinfo)
                {
                  const char *label = inp2.desc.c_str();
                  if (ImGui::MenuItem(label, nullptr,
                                      i.controller_type == inp2.id))
                  {
                    instance->core_changinpt(inp2.id, k);
                    loadcontconfig(true);
                  }
                }
                ImGui::EndMenu();
              }
            }
          }
        }
        ImGui::EndMenu();
      }
    }

    ImGui::EndMainMenuBar();
  }

  if (open_log)
    my_log.Draw("Core Log");

  ImVec2 maxSizedlg = ImVec2((float)io.DisplaySize.x * 0.7f, (float)io.DisplaySize.y * 0.7f);
  ImVec2 minSizedlg = ImVec2((float)io.DisplaySize.x * 0.4f, (float)io.DisplaySize.y * 0.4f);
  if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey", 32, minSizedlg, maxSizedlg))
  {
    // action if OK
    if (ImGuiFileDialog::Instance()->IsOk())
    {
      std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
      std::string filePath = ImGuiFileDialog::Instance()->GetCurrentPath();
      ImGuiFileDialog::Instance()->Close();
      coreselect = loadfile(instance, (char *)filePathName.c_str(), NULL, false);
      filenamepath = filePathName;
    }
    else
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
    std::vector<core_info> cores_info;
    cores_info.clear();
    bool found = false;

    for (auto &core : instance->cores)
    {

      if (core.no_roms && core.core_extensions == "")
        continue;

      std::string core_ext = core.core_extensions;
      std::string ext = filenamepath;
      ext = ext.substr(ext.find_last_of(".") + 1);
      std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
      std::transform(core_ext.begin(), core_ext.end(), core_ext.begin(), ::tolower);
      if (core_ext.find(ext) != std::string::npos)
      {
        hits++;
        cores_info.push_back(core);
        found = true;
      }
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

  if (ImGuiFileDialog::Instance()->Display("LoadSaveState", 32, minSizedlg, maxSizedlg))
  {
    // action if OK
    if (ImGuiFileDialog::Instance()->IsOk())
    {
      std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
      instance->core_savestate(filePathName.c_str(), false);
      // action
    }
    // close
    ImGuiFileDialog::Instance()->Close();
  }

  if (ImGuiFileDialog::Instance()->Display("SaveSaveState", 32, minSizedlg, maxSizedlg))
  {
    // action if OK
    if (ImGuiFileDialog::Instance()->IsOk())
    {
      std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
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
    if (!instance->core_inpbinds.at(0).inputbinds.size())
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
      if (instance->core_inpbinds.size() < 2)
      {
        int descnum = 1;
        for (auto &bind : instance->core_inpbinds[0].inputbinds)
        {
          size_t i = &bind - &instance->core_inpbinds[0].inputbinds.front();
          if (bind.description == "")
            continue;

          ImGui::TextWrapped("%s", bind.description.c_str());
          std::string script = "##" + bind.description;
          char *button_str = (char *)bind.joykey_desc.c_str();
          ImVec2 sz = ImGui::GetWindowSize();
          ImGui::SameLine(sz.x * 0.78);
          ImGui::SetNextItemWidth(sz.x * 0.2);
          ImGui::InputText(script.c_str(), button_str, 0, 0, NULL);
          if (ImGui::IsItemActive())
          {
            ImGui::SetWindowFocus();
            selected_inp = i;
            isselected_inp = true;
            selected_port = descnum - 1;
          }
        }
      }
      else if (ImGui::BeginTabBar("MyTabBar", ImGuiTabBarFlags_None))
      {
        int descnum = 1;
        for (auto &control : instance->core_inpbinds)
        {
          std::string descstring = "Player " + std::to_string(descnum);
          if (ImGui::BeginTabItem(descstring.c_str()))
          {
            for (auto &bind : control.inputbinds)
            {
              size_t i = &bind - &control.inputbinds.front();
              if (bind.description == "")
                continue;

              ImGui::TextWrapped("%s", bind.description.c_str());
              std::string script = "##" + bind.description;
              char *button_str = (char *)bind.joykey_desc.c_str();
              ImVec2 sz = ImGui::GetWindowSize();
              ImGui::SameLine(sz.x * 0.80);
              ImGui::SetNextItemWidth(sz.x * 0.2);
              ImGui::InputText(script.c_str(), button_str, 0, 0, NULL);
              if (ImGui::IsItemActive())
              {
                ImGui::SetWindowFocus();
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
        loadinpconf(instance->input_confcrc, true);
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

      for (auto &bind2 : instance->core_categories)
      {
        bool visible = false;
        for (auto &bind1 : instance->core_variables)
          if (bind1.category_name == bind2.key && bind1.config_visible)
            visible = true;
        if (visible)
          if (ImGui::TreeNode(bind2.desc.c_str()))
          {
            for (auto &bind : instance->core_variables)
            {
              std::string descript = bind.description;
              std::string hidden = "##" + descript;
              std::string var = bind.var;
              int sel_idx = bind.sel_idx;
              std::string current_item = bind.config_vals[sel_idx];
              bool checkbox_made = false;
              bool checkbox_enabled = false;

              if (bind2.key == bind.category_name && bind.config_visible)
              {
                for (izrange(j, IM_ARRAYSIZE(checkbox_allowable)))
                {
                  if (checkbox_enabled)
                    break;
                  for (auto &l : bind.config_vals)
                  {
                    if (stricmp(l.c_str(), checkbox_allowable[j]) == 0)
                    {
                      checkbox_made = true;
                      for (izrange(k, IM_ARRAYSIZE(true_vals)))
                        if (stricmp(var.c_str(), true_vals[k]) == 0)
                          checkbox_enabled = true;
                    }
                  }
                }

                if (checkbox_made)
                {

                  int total_w = descript.length();
                  ImGui::TextWrapped("%s", descript.c_str());
                  ImGui::SameLine();
                  HelpMarker(bind.tooltip.c_str());
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
                  ImGui::SameLine();
                  HelpMarker(bind.tooltip.c_str());
                  ImVec2 sz = ImGui::GetWindowSize();
                  ImGui::SameLine(sz.x * 0.65);
                  ImGui::SetNextItemWidth(sz.x * 0.3);
                  if (ImGui::BeginCombo(hidden.c_str(), current_item.c_str())) // The second parameter is the label previewed before opening the combo.
                  {
                    for (auto &config_val : bind.config_vals)
                    {
                      size_t n = &config_val - &bind.config_vals.front();
                      bool is_selected = (bind.sel_idx == n); // You can store your selection however you want, outside or inside your objects
                      if (ImGui::Selectable(config_val.c_str(), is_selected))
                      {
                        bind.sel_idx = n;
                        bind.var = config_val;
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
            ImGui::TreePop();
          }
      }

      for (auto &bind : instance->core_variables)
      {
        std::string descript = bind.description;
        std::string hidden = "##" + descript;
        std::string var = bind.var;
        int sel_idx = bind.sel_idx;
        std::string current_item = bind.config_vals[sel_idx];
        bool checkbox_made = false;
        bool checkbox_enabled = false;
        if (bind.category_name == "")
        {
          for (izrange(j, IM_ARRAYSIZE(checkbox_allowable)))
          {
            if (checkbox_enabled)
              break;
            for (izrange(l, bind.config_vals.size()))
            {
              if (stricmp(instance->v2_vars ? bind.config_vals[l].c_str() : bind.usevars.c_str(),
                          instance->v2_vars ? checkbox_allowable[j] : checkbox_allowablev1[j]) == 0)
              {
                checkbox_made = true;
                for (izrange(k, IM_ARRAYSIZE(true_vals)))
                  if (stricmp(var.c_str(), true_vals[k]) == 0)
                    checkbox_enabled = true;
              }
            }
          }

          if (checkbox_made)
          {

            int total_w = descript.length();
            ImGui::TextWrapped("%s", descript.c_str());
            if (bind.tooltip != "")
            {
              ImGui::SameLine();
              HelpMarker(bind.tooltip.c_str());
            }
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
            if (bind.tooltip != "")
            {
              ImGui::SameLine();
              HelpMarker(bind.tooltip.c_str());
            }
            ImVec2 sz = ImGui::GetWindowSize();
            ImGui::SameLine(sz.x * 0.65);
            ImGui::SetNextItemWidth(sz.x * 0.3);
            if (ImGui::BeginCombo(hidden.c_str(), current_item.c_str())) // The second parameter is the label previewed before opening the combo.
            {
              for (auto &config_val : bind.config_vals)
              {
                size_t n = &config_val - &bind.config_vals.front();
                bool is_selected = (bind.sel_idx == n); // You can store your selection however you want, outside or inside your objects
                if (ImGui::Selectable(config_val.c_str(), is_selected))
                {
                  bind.sel_idx = n;
                  bind.var = config_val;
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