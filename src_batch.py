import glob

sources = glob.glob('./src/main/*.cpp')
for i in sources:
    print(i)