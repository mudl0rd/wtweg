# WTFweg
> [!IMPORTANT]
> Contributions, issue/feature reports/requests will be ignored.

[libretro](https://www.libretro.com) code playset for anything with:

* x64 CPUs w/SSE4.1 and above.
* OGL 4.6
* Win10 and above/Linux
* GCC/G++ with C++17 support

## Where?

> [A list of tested cores is here](https://raw.githubusercontent.com/mudl0rd/WTFweg/master/cores.txt)
>
> [Cores can be grabbed here](http://buildbot.libretro.com/nightly/windows/x86_64/latest/)
> * "cores" is where all libretro cores go. 
>    * Cores on Windows can be transparently depacked from ZIP/RAR/7-Zip files. These can be in  the  "cores" directory as singular compressed files, or multiple core files as one pack of "cores.zip, "cores.rar", or "cores.7z", placed in the WTFweg install directory.
> * "saves" is where all saves are
> * "system" is where all BIOSes/system files go

## With?

> * [Dynamic rate control](https://docs.libretro.com/development/cores/dynamic-rate-control/).
> * Support for GL-based libretro cores.
> * Support for contentless libretro cores (like game engine reimplementations)
> * Support for transparently loading Windows libretro cores from ZIP/RAR/7z files.
> * Savestates/SRAM saving/loading.
> * [dear-imgui based UI](https://github.com/ocornut/imgui).
> * Commandline support for debugging cores with GDB.
> * Per-game settings per-core.
> * Multiple controller support.


## How?

* SDL2
* dear-imgui
* Meson
* MSYS2 (Windows), GCC/G++
* Visual Studio Code
