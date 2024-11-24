#include "godot_holoplay/HoloPlayVolume.h"

#include <gdextension_interface.h>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/engine.hpp>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <HoloPlayCore.h>

void initialize(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) return;

	hpc_client_error errco = hpc_InitializeApp("Godot HoloPlay Plugin", hpc_LICENSE_NONCOMMERCIAL);
	if (errco) {
		std::string errstr;
		switch (errco)
		{
			case hpc_CLIERR_NOSERVICE:
				errstr = "HoloPlay Service not running";
				break;
			case hpc_CLIERR_SERIALIZEERR:
				errstr = "Client message could not be serialized";
				break;
			case hpc_CLIERR_VERSIONERR:
				errstr = "Incompatible version of HoloPlay Service";
				break;
			case hpc_CLIERR_PIPEERROR:
				errstr = "Interprocess pipe broken";
				break;
			case hpc_CLIERR_SENDTIMEOUT:
				errstr = "Interprocess pipe send timeout";
				break;
			case hpc_CLIERR_RECVTIMEOUT:
				errstr = "Interprocess pipe receive timeout";
				break;
			default:
				errstr = "Unknown error";
				break;
		}
		ERR_PRINT("Could not connect to the HoloPlay service: " + String(errstr.c_str()) + " (" + String::num_int64(errco) + ").");
		return;
	}

	if (!glfwInit()) {
		ERR_PRINT("Could not initialize GLFW!");
		return;
	}

	if (!gladLoadGL()) {
		ERR_PRINT("Failed to initialize GLAD");
		return;
	}

	HoloPlayVolume::compile_shaders();
	GDREGISTER_CLASS(godot::HoloPlayVolume);
}

void uninitialize(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) return;

	HoloPlayVolume::free_shaders();
	glfwTerminate();
	hpc_CloseApp();
}

extern "C" {
	// Initialization.
	GDExtensionBool GDE_EXPORT gd_holoplay_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, const GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization) {
		godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

		init_obj.register_initializer(initialize);
		init_obj.register_terminator(uninitialize);
		init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

		return init_obj.init();
	}
}