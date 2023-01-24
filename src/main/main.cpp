#include <stdio.h>
#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"
#include "glad.h"
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <filesystem>
#include "clibretro.h"
#include "cmdline.h"
#include "utils.h"
#include "imgui_font.h"

#define WIDTH 1280
#define HEIGHT 720

SDL_DisplayMode dm;

void rendermenu(CLibretro *instance, SDL_Window *window, bool show_menu)
{
  std::string window_name;
  if (show_menu)
  {
    
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
    sdlggerat_menu(instance, &window_name);
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  }
  SDL_GL_SwapWindow(window);
}

int main2(const char *rom, const char *core, bool pergame)
{
  if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
  {
    printf("SDL_Init failed: %s\n", SDL_GetError());
    return 1;
  }

  SDL_GL_LoadLibrary(NULL);
  const char *glsl_version = "#version 330";
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
  SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE);
  SDL_Window *window = SDL_CreateWindow("WTFweg", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, window_flags);
  SDL_GLContext gl_context = SDL_GL_CreateContext(window);
  SDL_GL_MakeCurrent(window, gl_context);
  gladLoadGL();

  int window_indx = SDL_GetWindowDisplayIndex(window);
  float ddpi = -1;
  SDL_DisplayMode DM;
  SDL_GetCurrentDisplayMode(window_indx, &DM);
  SDL_GetDisplayDPI(window_indx, NULL, &ddpi, NULL);

  float dpi_scaling = ddpi / 72.f;
  SDL_Rect display_bounds;
  SDL_GetDisplayUsableBounds(window_indx, &display_bounds);
  int win_w = display_bounds.w * 7 / 8, win_h = display_bounds.h * 7 / 8;
  SDL_SetWindowSize(window, win_w, win_h);
  SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
  SDL_GetDesktopDisplayMode(window_indx, &dm);
  int swap = 1;
  swap = (int)dm.refresh_rate / (int)60;
  float refreshtarget = dm.refresh_rate / swap;
  float timing_skew = fabs(1.0f - 60 / refreshtarget);
  if (timing_skew <= 0.05)
    SDL_GL_SetSwapInterval((int)swap);
  else
    SDL_GL_SetSwapInterval(1);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.IniFilename = NULL;

  ImFontConfig font_cfg;
  font_cfg.FontDataOwnedByAtlas = false;
  io.Fonts->AddFontFromMemoryTTF((unsigned char *)Roboto_Regular, sizeof(Roboto_Regular), dpi_scaling * 12.0f, &font_cfg, io.Fonts->GetGlyphRangesJapanese());
  ImGuiStyle *style = &ImGui::GetStyle();
  style->ScaleAllSizes(dpi_scaling);
  ImGui::StyleColorsDark();
  ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
  ImGui_ImplOpenGL3_Init(glsl_version);

  std::filesystem::path p(get_wtfwegname());
  std::filesystem::path path = p.parent_path() / "gamecontrollerdb.txt";
  std::filesystem::path path2 = p.parent_path() / "mudmaps.txt";
  SDL_GameControllerAddMappingsFromFile(std::filesystem::absolute(path).string().c_str());
  SDL_GameControllerAddMappingsFromFile(std::filesystem::absolute(path2).string().c_str());

  auto instance = CLibretro::get_classinstance(window);

  // Main loop

  bool done = false;
  bool show_menu = true;
  reset_inp();

  SDL_Rect window_rect={0};

  if (rom && core)
    loadfile(instance.get(), rom, core, pergame);



  while (!done)
  {
    // Poll and handle events (inputs, window resize, etc.)
    // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
    // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
    // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
    // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
    SDL_Event event;
    while (SDL_PollEvent(&event) != 0)
    {

      ImGui_ImplSDL2_ProcessEvent(&event);
      if (event.type == SDL_QUIT)
        done = true;
      if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
        done = true;
      if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_F12)
      {
        if (instance->core_isrunning())
        {
          static bool window_fs = false;
          window_fs = !window_fs;

          if(window_fs)
          {
            SDL_GetWindowSize(window, &window_rect.w, &window_rect.h);
	          SDL_GetWindowPosition(window, &window_rect.x, &window_rect.y);
            int i = SDL_GetWindowDisplayIndex(window);
		        SDL_Rect j;
		        SDL_GetDisplayBounds(i, &j);
            SDL_SetWindowSize(window, j.w,j.h);
            SDL_SetWindowPosition(window, 0, 0);
            video_setsize(j.w, j.h);
            glViewport(0, 0, j.w, j.h);
            glScissor(0, 0, j.w, j.h);
          }
          else
          {
            	SDL_SetWindowSize(window, window_rect.w, window_rect.h);
	            SDL_SetWindowPosition(window, window_rect.x, window_rect.y);
              video_setsize(window_rect.w, window_rect.h);
              glViewport(0, 0, window_rect.w, window_rect.h);
              glScissor(0, 0, window_rect.w, window_rect.h);
          }
          SDL_SetWindowAlwaysOnTop(window,(SDL_bool)window_fs);
          SDL_SetWindowResizable(window, (SDL_bool)!window_fs);
          SDL_SetWindowBordered(window, (SDL_bool)!window_fs);
        }
        break;
      }

      if(event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_F2)
      instance->core_savestateslot(false);
      if(event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_F3)
      instance->core_savestateslot(true);

      if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_F1)
      {
       
        show_menu ^= true;
        SDL_SetRelativeMouseMode((SDL_bool)(!show_menu));
      }

      if (event.type == SDL_CONTROLLERDEVICEREMOVED)
      {
        close_inp(event.cdevice.which);
        SDL_GameControllerUpdate();
      }

      if (event.type == SDL_CONTROLLERDEVICEADDED)
      {
        init_inp(event.cdevice.which);
        SDL_GameControllerUpdate();
      }

      if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
      {
        int w = event.window.data1;
        int h = event.window.data2;
        if (instance->core_isrunning())
          video_setsize(w, h);

        glViewport(0, 0, w, h);
        glScissor(0, 0, w, h);
      }
      if (event.type == SDL_DROPFILE)
      {
        char *filez = (char *)event.drop.file;
        loadfile(instance.get(), event.drop.file, NULL, false);
        SDL_free(filez);
      }
    }

    

    glClearColor(0., 0., 0., 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (instance->core_isrunning())
    {
      video_bindfb();
      instance->core_run();
      video_render();
    }
    rendermenu(instance.get(), window, show_menu);
  }

  instance->core_unload();
  reset_inp();

  // Cleanup
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();
  SDL_GL_DeleteContext(gl_context);
  SDL_GL_UnloadLibrary();
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}

int main(int argc, char *argv[])
{

  if (argc > 2)
  {
    cmdline::parser a;
    a.add<std::string>("core_name", 'c', "core filename", true, "");
    a.add<std::string>("rom_name", 'r', "rom filename", true, "");
    a.add("pergame", 'g', "per-game configuration");
    a.parse_check(argc, argv);
    std::string rom = a.get<std::string>("rom_name");
    std::string core = a.get<std::string>("core_name");
    bool pergame = a.exist("pergame");
    if (!rom.empty() && !core.empty())
      return main2(rom.c_str(), core.c_str(), pergame);
    else
      printf("\nPress any key to continue....\n");
    return 0;
  }
  return main2(NULL, NULL, false);
}
