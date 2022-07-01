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

![cavestory](https://user-images.githubusercontent.com/56025978/176826673-3e7d9254-e6a6-4114-bb9c-81d0e26c0c1e.png)
![dinothawr](https://user-images.githubusercontent.com/56025978/176826700-83e7d83e-58cc-4895-913b-60c0d09dc082.png)
![xrick](https://user-images.githubusercontent.com/56025978/176826840-7c794157-6f74-4b68-8882-c89f0cb83b4e.png)


## How?

* SDL2
* dear-imgui
* Meson
* MSYS2 on Windows
* Visual Studio Code
