#include "ndi_input.h"


#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/os.hpp>

#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/string_name.hpp>

#include <Processing.NDI.Lib.h>



NDIInput::NDIInput()
{
	search_thread.instantiate();
	receive_thread.instantiate();

	set_create_texture(true);
}

NDIInput::~NDIInput()
{
	if (search_thread->is_started()) 
		search_thread->wait_to_finish();

	stop_receiving();

	if (finder)
		NDIlib_find_destroy(finder);
}



void NDIInput::set_create_texture(bool _create) 
{
	if (create_texture == _create) return;

	create_texture = _create;

	if (create_texture && (!texture.is_valid() || texture.is_null()))
		texture.instantiate();
	else if (!create_texture && (texture.is_valid() || !texture.is_null()))
		texture.unref();
	
	emit_signal("create_texture_changed", create_texture);
}

bool NDIInput::get_create_texture() const 
{
	return create_texture;
}

Ref<Texture2D> NDIInput::get_texture() const
{
	return texture;
}



void NDIInput::set_target_size (const Vector2i& _size)
{
	if (target_size == _size) return;

	receive_mutex.lock();
	target_size = _size;
	receive_mutex.unlock();
}

Vector2i NDIInput::get_target_size() const
{
	return target_size;
}



void NDIInput::search()
{
	if (is_search_running.is_set())
		return;
	
	search_thread->start(callable_mp(this, &NDIInput::_search_thread));
}



TypedArray<NDISource> NDIInput::_search_thread()
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



void NDIInput::_search_completed()
{
	TypedArray<NDISource> results = search_thread->wait_to_finish();

	emit_signal("search_completed", results);
}



void NDIInput::start_receiving(Ref<NDISource> _source) {
	UtilityFunctions::print("STARTED: ", receive_thread->is_started());

	if (!_source.is_valid() || _source.is_null() || current_source == _source) 
		return;

	if (is_receive_running.is_set())
		stop_receiving();

	UtilityFunctions::print("Start receiveing from: ", _source->get_name());

	current_source = _source;
	receive_should_exit.clear();
	receive_thread->start(callable_mp(this, &NDIInput::_receive_thread));
	UtilityFunctions::print("STARTED: ", receive_thread->is_started());
}



void NDIInput::_receive_thread()
{
	if (!current_source.is_valid() || current_source.is_null()) return;

	// CREATE RECEIVER
	NDIlib_recv_create_v3_t settings;
	settings.source_to_connect_to = current_source->handle;
	settings.color_format = NDIlib_recv_color_format_RGBX_RGBA;
	settings.p_ndi_recv_name = "Godot NDI Reciever";
	settings.bandwidth = NDIlib_recv_bandwidth_highest;
	settings.allow_video_fields = false;

	NDIlib_recv_instance_t receiver = NDIlib_recv_create_v3(&settings);
	
	if (!receiver) 
	{
		UtilityFunctions::push_error("Could not create NDI Receiver!");
		return;
	}

	// CONNECT TO SOURCE
	NDIlib_source_t handle = current_source->handle;
	NDIlib_recv_connect(receiver, &handle);
	UtilityFunctions::print("Connected to: ", current_source->get_name(), " :: ", current_source->get_url());

	// FRAME DATA
	bool has_valid_frame = false;
	NDIlib_video_frame_v2_t video;
	NDIlib_audio_frame_v3_t audio; 
	NDIlib_metadata_frame_t metadata;

	is_receive_running.set();

	call_deferred("emit_signal", "started");


	// MAIN LOOP
	while (!receive_should_exit.is_set())
	{
		has_valid_frame = false;

		NDIlib_frame_type_e frame_type = NDIlib_recv_capture_v3(receiver, &video, &audio, &metadata, 1000);
		
		switch (frame_type) 
		{
			case NDIlib_frame_type_video:
			{
				switch (video.FourCC) {
					case NDIlib_FourCC_video_type_RGBA:
					case NDIlib_FourCC_video_type_RGBX:
						has_valid_frame = true;
						break;
					
					default:
						UtilityFunctions::push_warning("Unsupported NDI video color format: ", video.FourCC);
				}

				if (!has_valid_frame) continue;

				// CREATE FRAME
				Ref<NDIVideoFrame> frame;
				frame.instantiate();

				frame->timecode = video.timecode;
				frame->timestamp = video.timestamp;

				frame->metadata = String(video.p_metadata);

				frame->frame_rate = (double) video.frame_rate_N / (double) video.frame_rate_N;
				frame->original_size = Vector2i(video.xres, video.yres);
				frame->original_aspect = (double) video.xres / (double) video.yres;

				// SIZE
				Vector2i current_size = Vector2i(video.xres, video.yres);

				receive_mutex.lock();
				Vector2i target = target_size;
				receive_mutex.unlock();

				// CREATE IMAGE
				size_t length = (size_t) video.line_stride_in_bytes * (size_t) video.yres;

				PackedByteArray buffer;
				buffer.resize(length);
				memcpy(buffer.ptrw(), video.p_data, length);

				// RESIZE
				if (target.x > 0 && target.y > 0 && current_size != target)
				{
					Vector2i offset = Vector2i();
					Vector2i resized = target;

					if (current_size.x > current_size.y)
					{
						resized.y = int(round(((float) target.x / (float) current_size.x) * (float) current_size.y));
						offset.y = round((target.y - resized.y) / 2.0);
					} 
					else if (current_size.y > current_size.x)
					{
						resized.x = int(round(((float) target.y / (float) current_size.y) * (float) current_size.x));
						offset.x = round((target.x - resized.x) / 2.0);
					}

					Ref<Image> image = Image::create_from_data(video.xres, video.yres, false, Image::FORMAT_RGBA8, buffer);
					image->resize(resized.x, resized.y, Image::INTERPOLATE_BILINEAR);

					Ref<Image> centered = Image::create(target.x, target.y, false, Image::FORMAT_RGBA8);
					centered->blit_rect(image, Rect2i(0, 0, resized.x, resized.y), offset);

					frame->image = centered;
				} 
				else
				{
					frame->image = Image::create_from_data(video.xres, video.yres, false, Image::FORMAT_RGBA8, buffer);
				}

				call_deferred("_video_frame_received", frame);
				
				NDIlib_recv_free_video_v2(receiver, &video); 
			} break;
				

			case NDIlib_frame_type_audio:
			{
				Ref<NDIAudioFrame> frame;
				frame.instantiate();

				frame->timecode = audio.timecode;
				frame->timestamp = audio.timestamp;

				frame->metadata = String(audio.p_metadata);

				frame->channel_count = audio.no_channels;
				frame->sample_count = audio.no_samples;
				frame->sample_rate = audio.sample_rate;
				
				// COPY SAMPLES
				int64_t length = audio.channel_stride_in_bytes * audio.no_channels;

				PackedByteArray buffer;
				buffer.resize(length);
				memcpy(buffer.ptrw(), audio.p_data, length);

				frame->samples = buffer.to_float32_array();
				
				call_deferred("_audio_frame_received", frame);

				NDIlib_recv_free_audio_v3(receiver, &audio);
			} break;
			

			case NDIlib_frame_type_metadata:
			{
				Ref<NDIMetaFrame> frame;
				frame.instantiate();

				frame->timecode = metadata.timecode;
				frame->metadata = String(metadata.p_data);
				
				call_deferred("_meta_frame_received", frame);

				NDIlib_recv_free_metadata(receiver, &metadata); 
			} break;
			

			//case NDIlib_frame_type_none:
			//	UtilityFunctions::print("No data...");
			//	break;
			
			//case NDIlib_frame_type_status_change:
			//	UtilityFunctions::print("Status change: ");
			//	break;
			
			case NDIlib_frame_type_error:
				UtilityFunctions::push_error("Error occured during receiving NDI frame from source: ", current_source->get_name());
				break;
		}
	}

	UtilityFunctions::print("Receiver exiting...");

	NDIlib_recv_destroy(receiver);

	is_receive_running.clear();
}



void NDIInput::_video_frame_received(Ref<NDIVideoFrame> _frame) 
{
	if (!_frame.is_valid() || _frame.is_null()) return;
	if (!_frame->get_image().is_valid() || _frame->get_image().is_null()) return;
	
	if (create_texture) {
		if (Vector2i(texture->get_size()) == _frame->get_image()->get_size())
		{
			texture->update(_frame->get_image());
		}
		else 
		{
			texture->set_image(_frame->get_image());
		}
	}

	emit_signal("video_frame_received", _frame);
}



void NDIInput::_audio_frame_received(Ref<NDIAudioFrame> _frame) 
{
	if (!_frame.is_valid() || _frame.is_null()) return;
	
	emit_signal("audio_frame_received", _frame);
}



void NDIInput::_meta_frame_received(Ref<NDIMetaFrame> _frame) 
{
	if (!_frame.is_valid() || _frame.is_null()) return;
	
	emit_signal("meta_frame_received", _frame);
}



void NDIInput::stop_receiving() {
	UtilityFunctions::print("STARTED: ", receive_thread->is_started());

	if (!is_receive_running.is_set()) return;

	UtilityFunctions::print("Stop receiving");
	
	receive_should_exit.set();
	receive_thread->wait_to_finish();

	current_source = Ref<NDISource>();

	emit_signal("stopped");
}



bool NDIInput::is_receiving() {
	return is_receive_running.is_set();
}



void NDIInput::_bind_methods()
{
	// METHODS
	ClassDB::bind_method(D_METHOD("search"), &NDIInput::search);
	ClassDB::bind_method(D_METHOD("start_receiving", "_source"), &NDIInput::start_receiving);
	ClassDB::bind_method(D_METHOD("stop_receiving"), &NDIInput::stop_receiving);
	ClassDB::bind_method(D_METHOD("is_receiving"), &NDIInput::is_receiving);
	
	ClassDB::bind_method(D_METHOD("set_target_size"), &NDIInput::set_target_size);
	ClassDB::bind_method(D_METHOD("get_target_size"), &NDIInput::get_target_size);
	
	ClassDB::bind_method(D_METHOD("get_create_texture"), &NDIInput::get_create_texture);
	ClassDB::bind_method(D_METHOD("set_create_texture"), &NDIInput::set_create_texture);
	ClassDB::bind_method(D_METHOD("get_texture"), &NDIInput::get_texture);
	
	ClassDB::bind_method(D_METHOD("_search_completed"), &NDIInput::_search_completed);
	
	ClassDB::bind_method(D_METHOD("_video_frame_received"), &NDIInput::_video_frame_received);
	ClassDB::bind_method(D_METHOD("_audio_frame_received"), &NDIInput::_audio_frame_received);
	ClassDB::bind_method(D_METHOD("_meta_frame_received"), &NDIInput::_meta_frame_received);

	// SIGNALS
	ADD_SIGNAL(MethodInfo("search_completed", PropertyInfo(Variant::ARRAY, "result", PROPERTY_HINT_ARRAY_TYPE, "NDISource")));

	ADD_SIGNAL(MethodInfo("started"));
	ADD_SIGNAL(MethodInfo("stopped"));

	ADD_SIGNAL(MethodInfo("create_texture_changed", PropertyInfo(Variant::BOOL, "creates")));

	ADD_SIGNAL(MethodInfo("video_frame_received", PropertyInfo(Variant::OBJECT, "_frame")));
	ADD_SIGNAL(MethodInfo("audio_frame_received", PropertyInfo(Variant::OBJECT, "_frame")));
	ADD_SIGNAL(MethodInfo("meta_frame_received", PropertyInfo(Variant::OBJECT, "_frame")));

	// PROPERTIES
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2I, "target_size", PROPERTY_HINT_NONE, "suffix:px"), "set_target_size", "get_target_size");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "create_texture", PROPERTY_HINT_NONE), "set_create_texture", "get_create_texture");
}


