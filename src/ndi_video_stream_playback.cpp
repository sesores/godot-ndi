
#include "ndi_video_stream_playback.h"

#include <godot_cpp/variant/utility_functions.hpp>

#include <Processing.NDI.Lib.h>



NDIVideoStreamPlayback::NDIVideoStreamPlayback () 
{
	receive_thread.instantiate();

	set_create_texture(true);
}

NDIVideoStreamPlayback::~NDIVideoStreamPlayback () 
{
	_stop();
}



void NDIVideoStreamPlayback::_stop() 
{
	UtilityFunctions::print("STARTED: ", receive_thread->is_started());

	if (!is_receive_running.is_set()) return;

	UtilityFunctions::print("Stop receiving");
	
	receive_should_exit.set();
	receive_thread->wait_to_finish();

	current_source = Ref<NDISource>();

	emit_signal("stopped");
}



void NDIVideoStreamPlayback::_play() 
{
	UtilityFunctions::print("STARTED: ", receive_thread->is_started());

	if (!current_source.is_valid() || current_source.is_null())
		UtilityFunctions::push_error("Before attempting to play, a valid instance of NDISource must be set!");
		return;

	if (is_receive_running.is_set())
		_stop();

	UtilityFunctions::print("Start receiveing from: ", current_source->get_name());

	receive_should_exit.clear();
	receive_thread->start(callable_mp(this, &NDIVideoStreamPlayback::_receive_thread));
	UtilityFunctions::print("STARTED: ", receive_thread->is_started());
}



bool NDIVideoStreamPlayback::_is_playing() const 
{ 
	return is_receive_running.is_set(); 
}



void NDIVideoStreamPlayback::_set_paused(bool _paused) 
{
	if (_paused && is_receive_running.is_set())
	{
		_stop();
	}
	else if (!_paused && !is_receive_running.is_set())
	{
		_play();
	}
}



bool NDIVideoStreamPlayback::_is_paused() const 
{ 
	return !is_receive_running.is_set(); 
}



double NDIVideoStreamPlayback::_get_length() const { return 0.0; }

double NDIVideoStreamPlayback::_get_playback_position() const { return 0.0; }

void NDIVideoStreamPlayback::_seek(double time) {}



void NDIVideoStreamPlayback::_set_audio_track(int32_t idx) 
{
}



Ref<Texture2D> NDIVideoStreamPlayback::_get_texture() const 
{ 
	return texture; 
}



void NDIVideoStreamPlayback::_update(double delta)
{
}



int32_t NDIVideoStreamPlayback::_get_channels() const 
{ 
	return 0; 
}



int32_t NDIVideoStreamPlayback::_get_mix_rate() const 
{ 
	return 0;
}



void NDIVideoStreamPlayback::set_create_texture(bool _create) 
{
	if (create_texture == _create) return;

	create_texture = _create;

	if (create_texture)
		texture.instantiate();
	else
		texture.unref();
	
	emit_signal("create_texture_changed", create_texture);
}

bool NDIVideoStreamPlayback::get_create_texture() const 
{
	return create_texture;
}



void NDIVideoStreamPlayback::set_target_size (const Vector2i& _size)
{
	if (target_size == _size) return;

	receive_mutex.lock();
	target_size = _size;
	receive_mutex.unlock();

	emit_signal("target_size_changed", target_size);
}

Vector2i NDIVideoStreamPlayback::get_target_size() const
{
	return target_size;
}




void NDIVideoStreamPlayback::set_source(Ref<NDISource> _source)
{
	if (current_source == _source) return;

	bool was_playing = _is_playing();

	if (_is_playing()) 
		_stop();
	
	current_source = _source;

	if (was_playing)
		_play();
}



Ref<NDISource> NDIVideoStreamPlayback::get_source() const
{
	return current_source;
}



void NDIVideoStreamPlayback::_receive_thread()
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

	call_deferred("emit_signal", "playing", current_source);

	// MAIN LOOP
	while (!receive_should_exit.is_set())
	{
		receive_mutex.lock();
		Vector2i target = target_size;
		receive_mutex.unlock();

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
						UtilityFunctions::push_warning("Unsupported video color format: ", video.FourCC);
						has_valid_frame = false;
						continue;
				}

				if (!has_valid_frame) continue;

				// CREATE FRAME
				Ref<NDIVideoFrame> frame;
				frame.instantiate();

				frame->timestamp = video.timestamp;
				frame->frame_rate = (double) video.frame_rate_N / (double) video.frame_rate_N;
				frame->original_size = Vector2i(video.xres, video.yres);

				// CREATE IMAGE
				int64_t length = video.data_size_in_bytes * video.yres;

				PackedByteArray buffer;
				buffer.resize(length);
				memcpy(buffer.ptrw(), video.p_data, length);

				frame->image = Image::create_from_data(video.xres, video.yres, false, Image::FORMAT_RGBA8, buffer);

				if (target.x > 0 && target.y > 0) 
				{
					frame->image->resize(target.x, target.y, Image::Interpolation::INTERPOLATE_BILINEAR);
				}

				call_deferred("_frame_received", frame);

				NDIlib_recv_free_video_v2(receiver, &video); 
			} break;
				
			case NDIlib_frame_type_audio:
				NDIlib_recv_free_audio_v3(receiver, &audio);
				break;
			
			case NDIlib_frame_type_metadata:
				UtilityFunctions::print("Meta: ", metadata.length);
				NDIlib_recv_free_metadata(receiver, &metadata); 
				break;
			
			case NDIlib_frame_type_none:
				UtilityFunctions::print("No data...");
				break;
			
			case NDIlib_frame_type_status_change:
				UtilityFunctions::print("Status change: ");
				break;
			
			case NDIlib_frame_type_error:
				UtilityFunctions::print("Error: ");
				break;
			
			default:
				continue;
		}
	}

	UtilityFunctions::print("Receiver exiting...");

	NDIlib_recv_destroy(receiver);

	is_receive_running.clear();
}



void NDIVideoStreamPlayback::_frame_received(Ref<NDIVideoFrame> _frame) 
{
	if (!_frame.is_valid() || _frame.is_null()) return;
	
	if (create_texture && _frame->get_image().is_valid()) {
		if (Vector2i(texture->get_size()) == _frame->get_image()->get_size())
		{
			texture->update(_frame->get_image());
		}
		else 
		{
			texture->set_image(_frame->get_image());
		}
	}

	emit_signal("frame_received", _frame);
}



void NDIVideoStreamPlayback::_bind_methods() 
{
	// METHODS
	ClassDB::bind_method(D_METHOD("set_source", "_source"), &NDIVideoStreamPlayback::set_source);
	ClassDB::bind_method(D_METHOD("get_source"), &NDIVideoStreamPlayback::get_source);
	
	ClassDB::bind_method(D_METHOD("set_target_size"), &NDIVideoStreamPlayback::set_target_size);
	ClassDB::bind_method(D_METHOD("get_target_size"), &NDIVideoStreamPlayback::get_target_size);
	
	ClassDB::bind_method(D_METHOD("get_create_texture"), &NDIVideoStreamPlayback::get_create_texture);
	ClassDB::bind_method(D_METHOD("set_create_texture"), &NDIVideoStreamPlayback::set_create_texture);

	ClassDB::bind_method(D_METHOD("_frame_received"), &NDIVideoStreamPlayback::_frame_received);
	
	// SIGNALS
	ADD_SIGNAL(MethodInfo("playing", PropertyInfo(Variant::OBJECT, "source")));
	ADD_SIGNAL(MethodInfo("stopped"));
	ADD_SIGNAL(MethodInfo("create_texture_changed", PropertyInfo(Variant::BOOL, "_creates")));
	ADD_SIGNAL(MethodInfo("target_size_changed", PropertyInfo(Variant::VECTOR2I, "_size")));
	ADD_SIGNAL(MethodInfo("frame_received", PropertyInfo(Variant::OBJECT, "frame")));

	// PROPERTIES
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "source", PROPERTY_HINT_NONE, ""), "set_source", "get_source");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2I, "target_size", PROPERTY_HINT_NONE, "suffix:px"), "set_target_size", "get_target_size");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "create_texture", PROPERTY_HINT_NONE, ""), "set_create_texture", "get_create_texture");
}


