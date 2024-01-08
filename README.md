# WTFweg

**Personal** [libretro](https://www.libretro.com) code playset for anything with:

* x86 CPUs w/SSE4.1 and above.
* OGL 4.1 and above
* Win10 and above/Linux
* GCC/G++ with C++17 support

This was developed due to very numerous longstanding issues I found with RetroArch as an application and as a project. I decided to take matters into my own hands and make a libretro core loading application to **my own exact needs**. As such, its not intended for use by anyone else. 

I also wanted to have a project where I can neatly work on things without interference. These include:

* Compression
* Emulation
* Graphics rendering
* Audio DSP and synthesis
* Low level Windows internals
* Cross platform development
* Optimization

Thus it neatly binds all my interests in one single mass of code.

## Where?

[A list of tested cores is here](https://raw.githubusercontent.com/cruduxcru0/WTFweg/master/cores.txt)

[Cores can be grabbed here](http://buildbot.libretro.com/nightly/windows/x86_64/latest/)

* "cores" is where all libretro cores go. 
    * Cores on Windows can be transparently depacked from ZIP/RAR/7-Zip files. These can be in the "cores" directory as singular compressed files, or multiple core files as one pack of "cores.zip, "cores.rar", or "cores.7z", placed in the WTFweg install directory.
* "saves" is where all saves are
* "system" is where all BIOSes/system files go

## With?

* [Dynamic rate control](https://docs.libretro.com/development/cores/dynamic-rate-control/).
* Support for GL-based libretro cores.
* Support for contentless libretro cores (like game engine reimplementations)
* Support for transparently loading Windows libretro cores from ZIP/RAR/7z files.
* Savestates/SRAM saving/loading.
* [dear-imgui based UI](https://github.com/ocornut/imgui).
* Commandline support for debugging cores with GDB.
* Per-game settings per-core.
* Multiple controller support.


## How?

* SDL2
* dear-imgui
* Meson
* MSYS2 on Windows
* Visual Studio Code
