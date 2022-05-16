# WTFweg

**Support/help/bug reports/pull requests will be ignored.**

## What?

Personal [libretro](https://www.libretro.com) code playset for anything with:

* **x86 CPUs w/SSE4.2+ 4 threads and above**.
* **ARM CPUs around RPI4 level supporting NEON**.
* **OGL 3.3/OGLESv2**
* Win7/modern Linux
* **Modern** GCC/G++ with C++17 support


## Tested cores (recommended for use):

* Snes9x
* Beetle PSX
* Parallel N64

## With?

* [Dynamic rate control](https://docs.libretro.com/development/cores/dynamic-rate-control/).
* OGL 3.3/ES2 based rendering w/ support for GL-based libretro cores.
* SDL2.0+ input/audio handling.
* Savestates/SRAM saving/loading.
* [dear-imgui based UI](https://github.com/ocornut/imgui).
* Commandline support for debugging cores with GDB.
* Per-game settings per-core.



![ss1](https://user-images.githubusercontent.com/56025978/163493614-c992cfd3-78d5-4579-87aa-53b580f70305.png)
![ss2](https://user-images.githubusercontent.com/56025978/163493616-6dd1bae6-6aab-4a64-9c20-88ece03bdd52.png)
![ss3](https://user-images.githubusercontent.com/56025978/163493617-5db73c9e-44f3-4caa-8283-57a17e90e0f3.png)

## How?

* OGL 3.3/ES2 (only)!
* C++17 using G++
* SDL2
* dear-imgui
* Meson
* MSYS2 on Windows
* Visual Studio Code
