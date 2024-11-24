#!python
import os

# Define our options
opts = Variables([], ARGUMENTS)
opts.Add(EnumVariable('target', "Compilation target", 'debug', ['d', 'debug', 'r', 'release']))
opts.Add(EnumVariable('platform', "Compilation platform", 'windows', ['', 'windows']))
opts.Add(PathVariable('target_path', 'The path where the lib is copied to.', 'build/bin/'))
opts.Add(PathVariable('target_name', 'The library name.', 'godot-libglass', PathVariable.PathAccept))

# Local dependency paths, adapt them to your setup
gd_headers_path = "godot-cpp/gdextension/"
gd_bindings_path = "godot-cpp/"
bits = 64

# env = DefaultEnvironment()
# Updates the environment with the option variables.
# Using godot-cpp's SConstruct environment is the easiest way to integrate it
env = SConscript("godot-cpp/SConstruct")
env = env.Clone()
print(os.environ.get('MSVC_VERSION', '0.0'))
opts.Update(env)

# Check platform
if 'p' in env: env['platform'] = env['p']
if env['platform'] == '':
    print("No valid target platform selected.")
    quit();

# For the reference:
# - CCFLAGS are compilation flags shared between C and C++
# - CFLAGS are for C-specific compilation flags
# - CXXFLAGS are for C++-specific compilation flags
# - CPPFLAGS are for pre-processor flags
# - CPPDEFINES are for pre-processor defines
# - LINKFLAGS are for linking flags

# Check our platform specifics
target_path: str
target_platform: str
if env['platform'] == 'windows':
    target_path = 'win64/'
    target_platform = 'windows'
    # This makes sure to keep the session environment variables on windows,
    # that way you can run scons in a vs 2017 prompt and it will find all the required tools
    # env.Append(ENV=os.environ)

    # env.Append(CPPDEFINES=['WIN32', '_WIN32', '_WINDOWS', '_CRT_SECURE_NO_WARNINGS'])
    # env.Append(CCFLAGS=['-W3', '-GR'])
    # env.Append(CXXFLAGS='/std:c++17')
    # if env['target'] in ('debug', 'd'):
    #     env.Append(CPPDEFINES=['_DEBUG'])
    #     env.Append(CCFLAGS=['-EHsc', '-MDd', '-ZI'])
    #     env.Append(LINKFLAGS=['-DEBUG'])
    # else:
    #     env.Append(CPPDEFINES=['NDEBUG'])
    #     env.Append(CCFLAGS=['-O2', '-EHsc', '-MD'])
else:
    print('This doesnt work on non-windows shit right now! I do not know how to use scons!')
    quit()

# Append debug state to name
is_debug = env['target'] in ('debug', 'd')

# make sure our binding library is properly includes
env.Append(CPPPATH=[
    'src/',
    'lib/HoloPlayCore/include/',
    'lib/glfw/include/',
    'lib/glad/include/',
    'lib/glad/src/'
])
env.Append(LIBS=[
    'lib/HoloPlayCore/HoloPlayCore.lib',
    'lib/glfw/glfw3.lib',
    'opengl32.lib',
    'user32.lib',
    'gdi32.lib',
    'shell32.lib'
])

# tweak this if you want to use different folders, or more folders, to store your source code in.
env.Append(CPPPATH=['src/'])
sources = []
sources += Glob('src/**/*.cpp')
sources += Glob('glad/src/glad.c')

library = env.SharedLibrary(f'build/bin/godot-libglass.{target_platform}.x{bits}{env["SHLIBSUFFIX"]}', source=sources)

Default(library)

# Generates help for the -h scons option.
Help(opts.GenerateHelpText(env))
