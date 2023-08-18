project('tutorial', 'c','cpp',default_options: [ 'buildtype=debug', 'c_std=c11','cpp_std=c++17'])
if host_machine.system() == 'linux'
sdl2dep = dependency('sdl2',
  required: true,
  static: false,
  method: 'pkg-config')
else
sdl2dep = dependency('sdl2',
  required: true,
  static: true,
  method: 'pkg-config')
endif

inc = [
  include_directories(
    'src/main',
    'src/deps/imgui',
    'src/deps',
    'src/deps/stb'
  )
]

src = []

srcdirs = [
    './src/main/',
    './src/main/mudutils',
    './src/deps/',
    './src/deps/imgui/'
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
  common_ldflags += [
    '-static'
    ]
add_project_link_arguments (
common_ldflags,
language: [ 'c', 'cpp' ],)
endif


dir_base = meson.current_source_dir()
dir_install = join_paths(dir_base, 'compile_dir')

executable('wtfweg',gui_app : true,install: true,install_dir: dir_install,include_directories : [inc],sources : [src],dependencies : [sdl2dep,dl_dep]
)

