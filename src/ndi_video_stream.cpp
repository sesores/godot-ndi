
#include "ndi_video_stream.h"


#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>

#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/string_name.hpp>

#include <Processing.NDI.Lib.h>



NDIVideoStream::NDIVideoStream()
{
	search_thread.instantiate();
}

NDIVideoStream::~NDIVideoStream()
{
	if (search_thread->is_started()) 
		search_thread->wait_to_finish();

	if (finder)
		NDIlib_find_destroy(finder);
}



void NDIVideoStream::search()
{
	if (is_search_running.is_set())
		return;
	
	search_thread->start(callable_mp(this, &NDIVideoStream::_search_thread));
}



TypedArray<NDISource> NDIVideoStream::_search_thread()
{
	is_search_running.set();

	UtilityFunctions::print("Searching for NDI sources...");

	if (finder)
		NDIlib_find_destroy(finder);
	
	finder = NDIlib_find_create_v2();

	if (!finder) 
	{
		is_search_running.clear();
		return TypedArray<NDISource>();
	}

	if (!NDIlib_find_wait_for_sources(finder, 2000))
	{
		UtilityFunctions::print("No change to the sources found.");
	}

	uint32_t count = 0;
	const NDIlib_source_t* current_sources = NDIlib_find_get_current_sources(finder, &count);
	UtilityFunctions::print("Network sources found:", count);
	
	TypedArray<NDISource> sources;
	sources.resize(count);

	for (uint32_t i = 0; i < count; i++)
	{
		UtilityFunctions::print(current_sources[i].p_ndi_name);
		
		Ref<NDISource> source;
		source.instantiate();

		source->handle = current_sources[i];
		source->name = String(source->handle.p_ndi_name);
		source->url = String(source->handle.p_url_address);

		sources[i] = source;
	}

	UtilityFunctions::print("Search exiting...");

	is_search_running.clear();

	call_deferred("_search_completed");

	return sources;
}

void NDIVideoStream::_search_completed()
{
	TypedArray<NDISource> results = search_thread->wait_to_finish();

	emit_signal("search_completed", results);
}



void NDIVideoStream::_bind_methods()
{
	// METHODS
	ClassDB::bind_method(D_METHOD("search"), &NDIVideoStream::search);
	ClassDB::bind_method(D_METHOD("_search_completed"), &NDIVideoStream::_search_completed);

	// SIGNALS
	ADD_SIGNAL(MethodInfo("search_completed", PropertyInfo(Variant::ARRAY, "result")));
}


