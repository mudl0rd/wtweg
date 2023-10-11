# WTFweg

**Personal** [libretro](https://www.libretro.com) code playset for anything with:

* **x86 CPUs w/SSE4.1 and above**.
* **OGL 4.6 and above**
* **Win8.1 and above/Linux**
* **GCC/G++ with C++17 support**
 
![6a0120a85dcdae970b0128776ff992970c-pi](https://github.com/cruduxcru0/WTFweg/assets/130935534/1fd7a5f8-2254-4ac6-8bf2-f8b5753cb0d7)


## Where?

[Cores can be grabbed here](http://buildbot.libretro.com/nightly/windows/x86_64/latest/)

[A list of tested cores is here](https://raw.githubusercontent.com/cruduxcru0/WTFweg/master/cores.txt)

* "cores" is where all libretro cores go. Cores on Windows can be transparently depacked from ZIPs.
* "saves" is where all saves are
* "system" is where all BIOSes/system files go

## Recommended cores for use with WTFweg:
* mame2003_plus_libretro.dll.zip (Arcade)
* mednafen_psx_libretro.dll.zip (PSX)
* mednafen_pce_libretro.dll.zip (PCE)
* mednafen_wswan_libretro.dll.zip (Wonderswan)
* mednafen_lynx_libretro.dll.zip (Lynx)
* mednafen_ngp_libretro.dll.zip (NeoGeo)
* mgba_libretro.dll.zip (GBA)
* sameboy_libretro.dll.zip (GB/GBC)
* kronos_libretro.dll.zip (Saturn)
* snes9x_libretro.dll.zip  (SNES)
* mupen64plus_next_libretro.dll.zip (N64)
* genesis_plus_gx_libretro.dll.zip (MD/Genesis)
* mesen_libretro.dll.zip (NES)
* pokemini_libretro.dll.zip (Pokemon Mini)
* gearcoleco_libretro.dll.zip (ColecoVision)

## With?

* [Dynamic rate control](https://docs.libretro.com/development/cores/dynamic-rate-control/).
* Support for GL-based libretro cores.
* Support for contentless libretro cores (like game engine reimplementations)
* Support for transparently loading Windows cores from ZIP files.
* Savestates/SRAM saving/loading.
* [dear-imgui based UI](https://github.com/ocornut/imgui).
* Commandline support for debugging cores with GDB.
* Per-game settings per-core.
* Multiple controller support.

![cavestory](https://user-images.githubusercontent.com/56025978/176826673-3e7d9254-e6a6-4114-bb9c-81d0e26c0c1e.png)
![dinothawr](https://user-images.githubusercontent.com/56025978/176826700-83e7d83e-58cc-4895-913b-60c0d09dc082.png)
![xrick](https://user-images.githubusercontent.com/56025978/176826840-7c794157-6f74-4b68-8882-c89f0cb83b4e.png)


## How?

* SDL2
* dear-imgui
* Meson
* MSYS2 on Windows
* Visual Studio Code
