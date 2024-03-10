
#include "register_types.h"

#include <gdextension_interface.h>

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

#include "ndi_input.h"
#include "ndi_source.h"
#include "ndi_frames.h"



using namespace godot;



void initialize_ndi_input_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	ClassDB::register_class<NDIInput>();
	ClassDB::register_class<NDISource>();

	ClassDB::register_class<NDIFrame>();
	ClassDB::register_class<NDIVideoFrame>();
	ClassDB::register_class<NDIAudioFrame>();
	ClassDB::register_class<NDIMetaFrame>();
}



void uninitialize_ndi_input_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}



extern "C" {

GDExtensionBool GDE_EXPORT ndi_input_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization) {
	godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

	init_obj.register_initializer(initialize_ndi_input_module);
	init_obj.register_terminator(uninitialize_ndi_input_module);
	init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

	return init_obj.init();
}

}
