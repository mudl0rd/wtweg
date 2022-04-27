project('tutorial', 'cpp',default_options: [ 'buildtype=debug', 'cpp_std=c++17'])
sdl2dep = dependency('sdl2',
  required: true,
  static: false,
  method: 'pkg-config')
inc = include_directories('src/main')
inc_imgui = include_directories('src/deps/imgui')
inc_deps = include_directories('src/deps')
inc_stb = include_directories('src/deps/stb')

glob = run_command('python3', 'src_batch.py')
src = glob.stdout().strip().split('\n')


dir_base = meson.current_source_dir()
dir_install = join_paths(dir_base, 'compile_dir')

executable('demo',install: true,install_dir: dir_install,include_directories : [inc,inc_imgui,inc_deps,inc_stb],sources : [src],dependencies : sdl2dep,
)

