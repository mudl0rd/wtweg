import glob

sources = glob.glob('./src/main/*.cpp')
sources += glob.glob('./src/main/mudutils/*.cpp')
sources += glob.glob('./src/deps/*.cpp')
sources += glob.glob('./src/deps/imgui/*.cpp')
for i in sources:
    print(i)