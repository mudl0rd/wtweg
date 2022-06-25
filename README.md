# WTFweg

## What?

Personal [libretro](https://www.libretro.com) code playset for anything with:

* **x86 CPUs w/SSE4.1 and above**.
* **ARM+NEON CPUs around RPI4 level and above**.
* **OGL 4.6/OGLESv2 and above**
* **Win7 and above/Linux**
* **GCC/G++ with C++17 support**


## Where?

**Note that these builds are radioactive-waste quality, beware!**

[Cores can be grabbed here](http://buildbot.libretro.com/nightly/windows/x86_64/latest/)

[A list of tested cores is here](https://raw.githubusercontent.com/mudlord/WTFweg/master/cores.txt)

[Releases compiled every Sunday, my time are here](https://www.mediafire.com/file/rp9ykqbevyobxa4/WTFweg.zip)

* "cores" is where all libretro cores go
* "saves" is where all saves are
* "system" is where all BIOSes/system files go


## With?

* [Dynamic rate control](https://docs.libretro.com/development/cores/dynamic-rate-control/).
* Support for GL-based libretro cores.
* Savestates/SRAM saving/loading.
* [dear-imgui based UI](https://github.com/ocornut/imgui).
* Commandline support for debugging cores with GDB.
* Per-game settings per-core.



![ss1](https://user-images.githubusercontent.com/56025978/163493614-c992cfd3-78d5-4579-87aa-53b580f70305.png)
![ss2](https://user-images.githubusercontent.com/56025978/163493616-6dd1bae6-6aab-4a64-9c20-88ece03bdd52.png)
![ss3](https://user-images.githubusercontent.com/56025978/163493617-5db73c9e-44f3-4caa-8283-57a17e90e0f3.png)

## How?

* SDL2
* dear-imgui
* Meson
* MSYS2 on Windows
* Visual Studio Code
