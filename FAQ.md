# WTFweg

## Why no support?

> * Designed for own personal usage, thus no issue reports/pull requests.
> * Prefer to do all code, at own pace, in own time.
> * See [this](https://aarongiles.com/dreamm/docs/v30/#faq-opensource) for ideas why no collab.

## Why?

> * **Because I am petty enough to do things for myself, my way.**
> * Experiment with things outside of "emulation" like:
>   - Compression.
>   - Audio DSP/synthesis.
>   - Graphics rendering.
>   - Software reverse engineering.

## Where?

> [A list of tested cores is here](https://raw.githubusercontent.com/mudl0rd/WTFweg/master/cores.txt)
>
> [Cores can be grabbed here](http://buildbot.libretro.com/nightly/windows/x86_64/latest/)
> * "cores" is where all libretro cores go. 
>    * Cores on Windows can be transparently depacked from ZIP/RAR/7-Zip files. These can be in  the  "cores" directory as singular compressed files, or multiple core files as one pack of "cores.zip, "cores.rar", or "cores.7z", placed in the WTFweg install directory.
> * "saves" is where all saves are
> * "system" is where all BIOSes/system files go
> 
> * "Emulation General" is a useful resource. Use it.

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
> * Drag-and-drop loading support for ROMs/ISOs, automatically choosing needed cores.

## How?

> * OGL4.6 (Windows/Linux), OGLES3 (RPI4/5)
> * SSE4.2/AVX2 (Windows/Linux), NEON (RPI4/5)
> * SDL2
> * dear-imgui
> * Meson
> * MSYS2 (Windows), GCC/G++ (Linux/RPI)
> * Visual Studio Code
