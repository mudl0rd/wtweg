#define IMGUI_DISABLE_DEMO_WINDOWS
#include "imgui.h"
#include "imgui_internal.h"
#include "ImGuiFileDialog.h"
#include "ImGuiFileDialogConfig.h"
#include <numeric>
#include <sstream>
#include "mudutils/utils.h"
#include "inout.h"
#include "clibretro.h"
#include "IconsForkAwesome.h"
#include "windows.h"

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
const char *checkbox_allowablev1[] = {"enabled|disabled", "disabled|enabled", "True|False", "False|True", "On|Off", "Off|On"};
const char *checkbox_allowable[] = {"enabled", "disabled", "True", "False", "On", "Off"};
const char *true_vals[] = {"enabled", "true", "on"};

static bool coreselect = false;
bool pergame_ = false;
bool cap_fps = true;
bool rombrowser = false;
struct FileRecord
{
  bool isDir = false;
  std::filesystem::path name;
  std::string showName;
  std::filesystem::path extension;
};
std::vector<FileRecord> fileRecords_;
std::filesystem::path pwd_;
std::string selected_path;

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

  float average(std::vector<float> const &v)
  {
    if (v.empty())
    {
      return 0;
    }

    auto const count = static_cast<float>(v.size());
    return std::reduce(v.begin(), v.end()) / count;
  }

  void Draw(const char *title, bool *p_open = NULL)
  {
    ImGuiIO &io = ImGui::GetIO();
    ImGui::SetNextWindowSizeConstraints(ImVec2(io.DisplaySize.x * 0.3f, io.DisplaySize.y * 0.5f),
                                        ImVec2(io.DisplaySize.x * 0.6f, io.DisplaySize.y * 0.7f));
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.2f, io.DisplaySize.y * 0.5f), ImGuiCond_Once, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowBgAlpha(0.5);

    if (!ImGui::Begin(title, p_open, ImGuiWindowFlags_NoCollapse))
    {
      ImGui::End();
      return;
    }

    CLibretro *core = CLibretro::get_classinstance();

    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 255, 0, 255));
    ImGui::Text("WTFweg average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
    static float low = 0;
    if (1000.0f / io.Framerate > low)
      low = 1000.0f / io.Framerate;
    ImGui::Text("WTFweg highest frametime %.3f ms/frame", low);
    ImGui::Text("Core samplerate (Hz): %.2f", core->core_samplerate);
    ImGui::Text("Core FPS limit: %.2f", core->core_fps);
    float avg = average(core->frames);
    ImGui::Text("Average libretro core runtime: %.3f ms/frame (%i frames)", avg, core->frameno);
    ImGui::Text("libretro core frametime graph:");
    ImGui::PopStyleColor();
    ImVec2 sz = ImGui::GetWindowSize();
    ImGui::PlotHistogram("Frametimes", &core->frames[0], core->frames.size(), 0, NULL, 0.0f, 50.0f, ImVec2(sz.x, 100));
    ImGui::Separator();
    bool clear = ImGui::Button("Clear");
    ImGui::SameLine();
    bool copy = ImGui::Button("Copy");
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
//  Usage:
//   static ExampleAppLog my_log;
//   my_log.AddLog("Hello %d world\n", 123);
//   my_log.Draw("title");

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

void loadfile(CLibretro *instance, clibretro_startoptions *options)
{
  ImGuiIO &io = ImGui::GetIO();
  pergame_ = options->game_specific_settings;
  cap_fps = options->framelimit;
  if (options->core != "")
  {

    instance->core_load(false, options);
  }
  else
  coreselect = true;
}

inline std::uint32_t GetDrivesBitMask()
{
  const DWORD mask = GetLogicalDrives();
  std::uint32_t ret = 0;
  for (int i = 0; i < 26; ++i)
  {
    if (!(mask & (1 << i)))
    {
      continue;
    }
    const char rootName[4] = {static_cast<char>('A' + i), ':', '\\', '\0'};
    const UINT type = GetDriveTypeA(rootName);
    if (type == DRIVE_REMOVABLE || type == DRIVE_FIXED || type == DRIVE_REMOTE)
    {
      ret |= (1 << i);
    }
  }
  return ret;
}

static uint32_t drives_ = GetDrivesBitMask();

template <class Functor>
struct ScopeGuard
{
  ScopeGuard(Functor &&t) : func(std::move(t)) {}

  ~ScopeGuard() { func(); }

private:
  Functor func;
};

bool HyperLink(const char *label, bool underlineWhenHoveredOnly = false)
{
  ImGuiStyle &style = ImGui::GetStyle();
  const ImU32 linkColor = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_TextDisabled]);
  const ImU32 linkHoverColor = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_Text]);
  const ImU32 linkFocusColor = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_Text]);

  const ImGuiID id = ImGui::GetID(label);

  ImGuiWindow *const window = ImGui::GetCurrentWindow();
  ImDrawList *const draw = ImGui::GetWindowDrawList();

  const ImVec2 pos(window->DC.CursorPos.x, window->DC.CursorPos.y + window->DC.CurrLineTextBaseOffset);
  const ImVec2 size = ImGui::CalcTextSize(label);
  ImRect bb(pos, {pos.x + size.x, pos.y + size.y});

  ImGui::ItemSize(bb, 0.0f);
  if (!ImGui::ItemAdd(bb, id))
    return false;

  bool isHovered = false;
  const bool isClicked = ImGui::ButtonBehavior(bb, id, &isHovered, nullptr);
  const bool isFocused = ImGui::IsItemFocused();

  const ImU32 color = isHovered ? linkHoverColor : isFocused ? linkFocusColor
                                                             : linkColor;

  draw->AddText(bb.Min, color, label);

  if (isFocused)
    draw->AddRect(bb.Min, bb.Max, color);
  else if (!underlineWhenHoveredOnly || isHovered)
    draw->AddLine({bb.Min.x, bb.Max.y}, bb.Max, color);

  return isClicked;
}

void rombrowse_setdir(std::string dir, CLibretro *instance)
{
  pwd_ = dir;
  auto rombrowse_update = [=]()
  {
    fileRecords_ = {FileRecord{true, "..", ICON_FK_FOLDER " ..", ""}};

    for (auto &p : std::filesystem::directory_iterator(pwd_))
    {
      FileRecord rcd = {FileRecord{false, "", "", ""}};

      if (p.is_regular_file())
      {
        rcd.isDir = false;
      }
      else if (p.is_directory())
      {
        rcd.isDir = true;
      }
      else
      {
        continue;
      }

      rcd.name = p.path().filename();
      if (rcd.name.empty())
        continue;
      std::string str;
      if (!rcd.isDir)
      {
        rcd.extension = p.path().filename().extension();
        if (rcd.extension.empty())
          continue;
        std::string str2 = rcd.extension.string();
        str2.erase(str2.begin());
        bool ismusicfile = (instance->coreexts.find(str2) != std::string::npos);
        if (!ismusicfile)
          continue;
        else
          str = ICON_FK_GAMEPAD " ";
      }
      else
        str = ICON_FK_FOLDER " ";
      rcd.showName = str + p.path().filename().string();
      fileRecords_.push_back(rcd);
    }
    std::sort(fileRecords_.begin(), fileRecords_.end(),
              [](const FileRecord &L, const FileRecord &R)
              {
                return (L.isDir ^ R.isDir) ? L.isDir : (L.name < R.name);
              });
  };
  rombrowse_update();
}

void sdlggerat_menu(CLibretro *instance, std::string *window_str)
{

  static bool inputsettings = false;
  static bool coresettings = false;
  static bool aboutbox = false;
  static bool load_core = false;
  static bool no_cores = false;
  static bool open_log = false;
  static bool subsys_box = false;
  static int subsys_coreindex = 0;
  ImVec2 winsize = ImVec2(0, 0);
  static std::vector<std::string> rompaths(10, std::string("None"));
  static std::string addonloader = "LoadAddon";
  static bool addonloader_pick = false;

  auto rombrowse_update = [=]()
  {
    fileRecords_ = {FileRecord{true, "..", ICON_FK_FOLDER " ..", ""}};

    for (auto &p : std::filesystem::directory_iterator(pwd_))
    {
      FileRecord rcd = {FileRecord{false, "", "", ""}};

      if (p.is_regular_file())
      {
        rcd.isDir = false;
      }
      else if (p.is_directory())
      {
        rcd.isDir = true;
      }
      else
      {
        continue;
      }

      rcd.name = p.path().filename();
      if (rcd.name.empty())
        continue;
      std::string str;
      if (!rcd.isDir)
      {
        rcd.extension = p.path().filename().extension();
        if (rcd.extension.empty())
          continue;
        std::string str2 = rcd.extension.string();
        str2.erase(str2.begin());
        bool ismusicfile = (instance->coreexts.find(str2) != std::string::npos);
        if (!ismusicfile)
          continue;
        else
          str = ICON_FK_GAMEPAD " ";
      }
      else
        str = ICON_FK_FOLDER " ";
      rcd.showName = str + p.path().filename().string();
      fileRecords_.push_back(rcd);
    }
    std::sort(fileRecords_.begin(), fileRecords_.end(),
              [](const FileRecord &L, const FileRecord &R)
              {
                return (L.isDir ^ R.isDir) ? L.isDir : (L.name < R.name);
              });
  };

  auto rombrowse_browse = [=](int width, int height)
  {
    static std::string selected_fname;
    bool updrecs = false;
    ImGuiIO &io = ImGui::GetIO();

    ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y - height));
    ImGui::SetNextWindowPos(ImVec2(0, height));
    ImGui::Begin("ROM browse", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_HorizontalScrollbar);
    float reserveHeight = io.DisplaySize.y - height;
    const char currentDrive = static_cast<char>(pwd_.c_str()[0]);
    const char driveStr[] = {currentDrive, ':', '\0'};

    ImGui::PushItemWidth(4 * ImGui::GetFontSize());
    if (ImGui::BeginCombo("##select_drive", driveStr))
    {
      ScopeGuard guard([&]
                       { ImGui::EndCombo(); });

      for (int i = 0; i < 26; ++i)
      {
        if (!(drives_ & (1 << i)))
        {
          continue;
        }

        const char driveCh = static_cast<char>('A' + i);
        const char selectableStr[] = {driveCh, ':', '\0'};
        const bool selected = currentDrive == driveCh;

        if (ImGui::Selectable(selectableStr, selected) && !selected)
        {
          char newPwd[] = {driveCh, ':', '\\', '\0'};
          std::filesystem::path pah(newPwd);
          rombrowse_setdir(pah.string(), instance);
        }
      }
    }
    ImGui::PopItemWidth();

    int secIdx = 0, newDirLastSecIdx = -1;
    for (const auto &sec : pwd_)
    {
#ifdef _WIN32
      if (secIdx == 1)
      {
        ++secIdx;
        continue;
      }
#endif

      ImGui::PushID(secIdx);
      if (secIdx > 0)
      {
        ImGui::SameLine();
        ImGui::Text("/");
        ImGui::SameLine();
      }
      if (HyperLink(sec.u8string().c_str()))
      {
        newDirLastSecIdx = secIdx;
      }
      ImGui::PopID();

      ++secIdx;
    }

    if (newDirLastSecIdx >= 0)
    {
      int i = 0;
      std::filesystem::path dstDir;
      for (const auto &sec : pwd_)
      {
        if (i++ > newDirLastSecIdx)
        {
          break;
        }
        dstDir /= sec;
      }

#ifdef _WIN32
      if (newDirLastSecIdx == 0)
      {
        dstDir /= "\\";
      }
#endif
      pwd_ = dstDir;
      updrecs = true;
    }

    float panelHeight = ImGui::GetContentRegionAvail().y;
    float cellSize = ImGui::GetTextLineHeight();
    int items_sz = fileRecords_.size() * cellSize;
    int columns = (int)(items_sz / (int)panelHeight) + 1;
    if (columns <= 0)
      columns = 1;
    float items = 0;
    ImGui::Columns(columns, 0, false);

    for (auto &rsc : fileRecords_)
    {

      if (!rsc.name.empty() && rsc.name.c_str()[0] == '$')
        continue;
      bool selected = rsc.showName == selected_fname;
      ImGui::Selectable(rsc.showName.c_str(), selected,
                        ImGuiSelectableFlags_DontClosePopups);
      if (ImGui::IsItemClicked(0) && ImGui::IsMouseDoubleClicked(0))
      {
        if (rsc.isDir)
        {
          pwd_ = (rsc.name != "..") ? (pwd_ / rsc.name) : pwd_.parent_path();
          updrecs = true;
        }
        else
        {
          std::string path2 = std::filesystem::path(std::filesystem::canonical(pwd_) / rsc.name).string();
          selected_fname = rsc.showName;
          selected_path = path2;
          rombrowser = false;
          clibretro_startoptions options;
          options.rompaths.clear();
          options.rompaths.push_back(selected_fname);
          options.usesubsys = false;
          options.framelimit = cap_fps;
          options.game_specific_settings = pergame_;
          options.savestate = "";
          options.core = "";
          loadfile(instance, &options);
        }
      }
      items += cellSize;
      if (items <= panelHeight)
      {
        items = 0;
        ImGui::NextColumn();
      }
    }
    ImGui::End();

    // Rendering
    if (updrecs)
    {
      rombrowse_update();
      updrecs = false;
    }
  };

  ImGuiIO &io = ImGui::GetIO();

  if (ImGui::BeginMainMenuBar())
  {
    if (ImGui::BeginMenu("File"))
    {
      if (ImGui::MenuItem("Load content (ROM/ISO/etc)"))
      {
        if (instance->coreexts == "")
        {
          no_cores = true;
        }
        else

          ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", " Choose a ROM/ISO", instance->coreexts.c_str(), ".", "", 1, nullptr, flags);
      }

      bool cont = false;
      for (auto &core : instance->cores)
        if (core.subsystems.size())
          cont = true;
      if (cont)
      {
        if (ImGui::BeginMenu("Load addon content (ROMs/ISOs/etc)"))
        {
          for (auto &core : instance->cores)
          {
            size_t j = &core - &instance->cores.front();
            if (core.subsystems.size())
            {
              if (ImGui::MenuItem(core.core_name.c_str()))
              {
                subsys_box = true;
                subsys_coreindex = j;
              }
            }
          }
          ImGui::EndMenu();
        }
      }

      if (ImGui::MenuItem("Content browser (ROM/ISO/etc)", nullptr,
                          rombrowser == true))
        rombrowser = !rombrowser;

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
          ImGuiFileDialog::Instance()->OpenDialog("LoadSaveState", "Load a savestate", ".state", ".");

        if (ImGui::MenuItem("Save Savestate"))
          ImGuiFileDialog::Instance()->OpenDialog("SaveSaveState", "Save a savestate", ".state", ".", "", 1, IGFDUserDatas("SaveFile"), ImGuiFileDialogFlags_ConfirmOverwrite);
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
    else
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

        ImGui::Separator();

        if (ImGui::MenuItem("Cap FPS to core limit", nullptr, cap_fps == true))
        {
          cap_fps = !cap_fps;
          instance->framecap(cap_fps);
        }

        if (ImGui::MenuItem("Developer Window", nullptr,
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
                    instance->loadcontconfig(true);
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
    winsize = ImGui::GetWindowSize();
    ImGui::EndMainMenuBar();
  }

  if (rombrowser)
    rombrowse_browse(winsize.x, winsize.y);

  if (open_log)
    my_log.Draw("Developer Window");

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
      clibretro_startoptions options;
      options.rompaths.clear();
      options.rompaths.push_back(filePathName);
      options.usesubsys = false;
      options.framelimit = cap_fps;
      options.game_specific_settings = pergame_;
      options.savestate = "";
      options.core = "";
      selected_path = filePathName;
      loadfile(instance, &options);
    }
    else
      ImGuiFileDialog::Instance()->Close();
  }

  if (no_cores)
  {
    popup_widget(&no_cores, "No libretro cores", "There is no ROM/ISO loading cores detected.");
    return;
  }

  if (subsys_box)
    ImGui::OpenPopup("Select addon content to load");

  ImGui::SetNextWindowSizeConstraints(ImVec2(io.DisplaySize.x * 0.3f, io.DisplaySize.y * 0.3f),
                                      ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f));
  ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
  if (ImGui::BeginPopupModal("Select addon content to load", &subsys_box, ImGuiWindowFlags_AlwaysAutoResize))
  {
    static size_t sel_indx = 0;

    for (auto &core : instance->cores)
    {
      size_t k = &core - &instance->cores.front();
      static bool is_selected = false;

      if (k == subsys_coreindex)
      {
        if (ImGui::BeginCombo("Subsystem", core.subsystems[sel_indx].subsystem_name.c_str())) // The second parameter is the label previewed before opening the combo.
        {
          for (auto &config_val : core.subsystems)
          {
            size_t n = &config_val - &core.subsystems.front();
            is_selected = (sel_indx == n); // You can store your selection however you want, outside or inside your objects
            if (ImGui::Selectable(config_val.subsystem_name.c_str(), is_selected))
            {
              rompaths.clear();
              rompaths.resize(core.subsystems[n].rominfo.size());
              std::fill(rompaths.begin(), rompaths.end(), "None");
              sel_indx = n;
            }
          }
          ImGui::EndCombo();
        }

        for (auto &j : core.subsystems[sel_indx].rominfo)
        {
          size_t n = &j - &core.subsystems[sel_indx].rominfo.front();
          char rompath[255] = {0};
          strcpy(rompath, rompaths[n].c_str());
          std::string str = j.romtype;
          if (j.required)
            str += " (required)";
          ImGui::Text(str.c_str());
          std::string idtype = "##load" + std::to_string(n);
          ImGui::InputText(idtype.c_str(), rompath, 255, ImGuiInputTextFlags_ReadOnly);
          ImGui::SameLine();
          std::string load = "Load##" + std::to_string(n);
          if (ImGui::Button(load.c_str()))
          {
            std::string coreexts = j.romtype + " {." +
                                   MudUtil::replace_all(j.romexts, "|", ",.") + "}";
            addonloader = std::to_string(n);
            subsys_box = false;
            ImGuiFileDialog::Instance()->OpenDialog(addonloader.c_str(), "Load a addon file", coreexts.c_str(), ".", 1, nullptr,
                                                    ImGuiFileDialogFlags_Modal);
          }
        }
      }
    }

    if (ImGui::Button("OK"))
    {
      for (auto &core : instance->cores)
      {
        size_t k = &core - &instance->cores.front();
        if (k == subsys_coreindex)
        {
          clibretro_startoptions options = {0};
          options.rompaths = rompaths;
          options.usesubsys = true;
          options.current_subsystem = core.subsystems[sel_indx];
          options.framelimit = cap_fps;
          options.game_specific_settings = pergame_;
          options.savestate = "";
          options.core = core.core_path;
          instance->core_load(false, &options);
          subsys_box = false;
          return;
        }
      }

      int subtype =
          subsys_box = false;
    }
    ImGui::Bullet();
    ImGui::SameLine();
    ImGui::TextWrapped("Pick the addon content you want to load.");
    ImGui::EndPopup();
  }

  if (ImGuiFileDialog::Instance()->Display(addonloader.c_str(), 32, minSizedlg, maxSizedlg))
  {
    // action if OK
    if (ImGuiFileDialog::Instance()->IsOk())
    {
      int romid = std::stoi(addonloader);
      std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
      std::string filePath = ImGuiFileDialog::Instance()->GetCurrentPath();
      ImGuiFileDialog::Instance()->Close();
      rompaths.at(romid) = filePathName;
    }
    else
      ImGuiFileDialog::Instance()->Close();
    subsys_box = true;
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
        clibretro_startoptions options = {0};
        options.rompaths.clear();
        options.rompaths.push_back("");
        options.framelimit = cap_fps;
        options.game_specific_settings = pergame_;
        options.savestate = "";
        options.current_core = cores_info.at(listbox_item_current);
        options.core = cores_info.at(listbox_item_current).core_path;
        instance->core_load(false, &options);
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

  if(coreselect)
  {
    clibretro_startoptions options = {0};
    options.rompaths.clear();
    options.rompaths.push_back(selected_path);
    options.usesubsys = false;
    options.framelimit = cap_fps;
    options.game_specific_settings = pergame_;
    options.savestate = "";
    options.core = "";

    coreselect = true;
    int hits = 0;
    std::vector<core_info> cores_info;
    cores_info.clear();
    bool found = false;

    for (auto &core : instance->cores)
    {

      if (core.no_roms && core.core_extensions == "")
        continue;

      std::string core_ext = core.core_extensions;
      std::string ext = options.rompaths[0];
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
      options.core = cores_info.at(0).core_path;
      options.current_core = cores_info.at(0);
      instance->core_load(false, &options);
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
        options.core = cores_info.at(listbox_item_current).core_path;
        options.current_core = cores_info.at(listbox_item_current);
        instance->core_load(false, &options);
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

  if (inputsettings)
  {
    static int selected_inp = 0;
    static bool isselected_inp = false;
    static int selected_port = 0;
    instance->checkbuttons_forui(selected_inp, &isselected_inp, selected_port);
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

      auto inpcode = [](coreinput_bind &bind, int i)
      {
        if (bind.description == "")
          return;
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
          selected_port = 0;
        }
      };

      if (ImGui::BeginTabBar("MyTabBar", ImGuiTabBarFlags_None))
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
              inpcode(bind, i);
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
        instance->loadinpconf(instance->input_confcrc, true);
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

      auto corecode = [](CLibretro *instance, loadedcore_configvars &bind)
      {
        bool checkbox_made = false;
        bool checkbox_enabled = false;
        for (izrange(j, IM_ARRAYSIZE(checkbox_allowable)))
        {
          if (checkbox_enabled)
            break;
          for (izrange(l, bind.config_vals.size()))
          {

            if (stricmp(instance->v2_vars ? bind.config_vals[l].c_str() : bind.usevars.c_str(),
                        instance->v2_vars ? checkbox_allowable[j] : checkbox_allowablev1[j]) == 0)
            {
              for (izrange(k, IM_ARRAYSIZE(true_vals)))
                if (stricmp(instance->v2_vars ? bind.config_vals[l].c_str() : bind.usevars.c_str(), true_vals[k]) == 0)
                  checkbox_made = true;

              for (izrange(k, IM_ARRAYSIZE(true_vals)))
                if (stricmp(bind.var.c_str(), true_vals[k]) == 0)
                  checkbox_enabled = true;
            }
          }
        }

        std::string descript = bind.description;
        std::string hidden = "##" + descript;
        int sel_idx = bind.sel_idx;
        std::string current_item = bind.config_vals[sel_idx];
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
      };

      for (auto &bind2 : instance->core_categories)
      {
        bool visible = false;
        for (auto &bind1 : instance->core_variables)
        {
          if (bind1.category_name == bind2.key && bind1.config_visible)
          {
            visible = true;
            break;
          }
        }
        if (visible)
          if (ImGui::TreeNode(bind2.desc.c_str()))
          {
            for (auto &bind : instance->core_variables)
            {
              std::string var = bind.var;

              if (bind2.key == bind.category_name && bind.config_visible)
                corecode(instance, bind);
            }
            ImGui::TreePop();
          }
      }
      // do non categorized vars.
      for (auto &bind : instance->core_variables)
      {
        std::string var = bind.var;
        if (bind.category_name == "")
          corecode(instance, bind);
      }
      // click ok when finished adjusting
      if (ImGui::Button("OK"))
      {
        coresettings = false;
        instance->load_coresettings(true);
      }

      ImGui::EndPopup();
    }
  }
}