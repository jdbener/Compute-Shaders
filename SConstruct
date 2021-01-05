import glob, os;

env = DefaultEnvironment()
env['CC'] = 'clang'
env['CXX'] = 'clang++'
env['CXXFLAGS'] = '-std=c++14 -g'
if False: # Leak checking
    env['CXXFLAGS'] += ' -fsanitize=address'
    env['LINKFLAGS'] = '-fsanitize=address'


# Automatically determine the sources
sources = glob.glob("**/*.cpp", recursive=True)
sources += glob.glob("**/*.c", recursive=True)

# Use pkg-config to link against glfw and glew
env.ParseConfig("pkg-config glfw3 --cflags --libs")
env.ParseConfig("pkg-config glew --cflags --libs")

# Compile
env.Program(target = "run", source = sources)



# Automatically find all of the test shaders
shaders = glob.glob("**/*.glsl", recursive=True)
