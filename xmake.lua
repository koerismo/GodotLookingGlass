add_rules("mode.debug", "mode.release")
set_languages("c99", "c++17")
add_requires("glfw 3.4")
add_defines("TYPED_METHOD_BIND")

target("LibGlass")
    set_kind("shared")
    add_packages("glfw")
    add_files("src/*.cpp", "src/godot_holoplay/*.cpp", "lib/glad/src/glad.c")
    add_links("lib/HoloPlayCore/HoloPlayCore.lib")
    add_links("godot-cpp/bin/libgodot-cpp.windows.template_debug.x86_64.lib")
    add_includedirs("lib/glad/include", "lib/glad/src")
    add_includedirs("lib/HoloPlayCore/include")
    add_includedirs("godot-cpp/include", "godot-cpp/gen/include", "godot-cpp/gdextension")
