project('tutorial', 'c','cpp',default_options: [ 'buildtype=debug', 'c_std=c11','cpp_std=c++17'])
if host_machine.system() == 'linux'
sdl2dep = dependency('sdl2',
  required: true,
  static: false,
  method: 'pkg-config')
else
sdl2dep = dependency('sdl2',
  required: true,
  static: false,
  method: 'pkg-config')
endif

inc = [
  include_directories(
    'src/main',
    'src/deps/imgui',
    'src/deps/misc',
    'src/deps/stb',
    'src/deps/fex',
  )
]

src = []
src += [
	  './src/deps/fex/7z_C/LzmaLib.c',
    './src/deps/fex/7z_C/LzmaEnc.c',
    './src/deps/fex/7z_C/LzmaDec.c',
    './src/deps/fex/7z_C/Lzma86Enc.c',
    './src/deps/fex/7z_C/Lzma86Dec.c',
    './src/deps/fex/7z_C/Lzma2Enc.c',
    './src/deps/fex/7z_C/Lzma2Dec.c',
    './src/deps/fex/7z_C/LzFind.c',
    './src/deps/fex/7z_C/Delta.c',
    './src/deps/fex/7z_C/CpuArch.c',
    './src/deps/fex/7z_C/BraIA64.c',
    './src/deps/fex/7z_C/Bra86.c',
    './src/deps/fex/7z_C/Bra.c',
    './src/deps/fex/7z_C/Bcj2.c',
    './src/deps/fex/7z_C/Alloc.c',
    './src/deps/fex/7z_C/7zStream.c',
    './src/deps/fex/7z_C/7zIn.c',
    './src/deps/fex/7z_C/Ppmd7Enc.c',
    './src/deps/fex/7z_C/Ppmd7Dec.c',
    './src/deps/fex/7z_C/Ppmd7.c',
    './src/deps/fex/7z_C/7zDec.c',
    './src/deps/fex/7z_C/7zCrcOpt.c',
    './src/deps/fex/7z_C/7zCrc.c',
    './src/deps/fex/7z_C/7zBuf2.c',
    './src/deps/fex/7z_C/7zBuf.c',
    './src/deps/fex/7z_C/7zAlloc.c',
    './src/deps/fex/7z_C/MtCoder.c',
    './src/deps/fex/7z_C/LzFindMt.c',
    './src/deps/fex/7z_C/Threads.c'

]

srcdirs = [
    './src/main/',
    './src/main/mudutils',
    './src/deps/misc',
    './src/deps/imgui/',
    './src/deps/fex/fex',
    './src/deps/fex/unrar/'
]

foreach srcitem : srcdirs
  glob = run_command('python3', 'src_batch.py',srcitem)
  src += glob.stdout().strip().split('\n')
endforeach



test_ldflags = []
common_ldflags = meson.get_compiler('cpp').get_supported_link_arguments (
  test_ldflags)

if host_machine.system() == 'linux'
    dl_dep = meson.get_compiler('cpp').find_library('dl', required: true)
else
    dl_dep = dependency('', required: false)




if get_option('buildtype') == 'debug'
  add_global_arguments('-DDEBUG', language: [ 'c', 'cpp' ])
endif

add_project_link_arguments (
common_ldflags,
language: [ 'c', 'cpp' ],)
endif
add_project_link_arguments('-Wl,-Map=wtfweg.map', language : ['c','cpp'])

if host_machine.system() == 'windows'
 add_project_link_arguments('-lwinmm', language : ['c','cpp'])
 endif

progdefs = []

if host_machine.cpu() == 'aarch64'
  progdefs += ['-DUSE_RPI']
endif

dir_base = meson.current_source_dir()
dir_install = join_paths(dir_base, 'compile_dir')
executable('wtfweg',gui_app : true,c_args: [progdefs],cpp_args: [progdefs], install: true,install_dir: dir_install,include_directories : [inc],sources : [src],dependencies : [sdl2dep,dl_dep]
)

