project('tutorial', 'cpp',default_options: ['default_library=static', 'buildtype=debug', 'cpp_std=c++17'])
sdl2dep = dependency('sdl2',
  required: true,
  static: true,
  method: 'pkg-config')
inc = include_directories('src/main')
inc_imgui = include_directories('src/deps/imgui')
inc_deps = include_directories('src/deps')
inc_stb = include_directories('src/deps/stb')

glob = run_command('python3', 'src_batch.py')
src = glob.stdout().strip().split('\n')

executable('demo',link_args : '-static', include_directories : [inc,inc_imgui,inc_deps,inc_stb],sources : [src],dependencies : sdl2dep,

)


