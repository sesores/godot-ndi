#include "video_stream_ndi.h"


#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>

#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/string_name.hpp>

#include <Processing.NDI.Lib.h>



// NDI SOURCE
NDISource::NDISource() {}

String NDISource::get_name() const { return name; }
String NDISource::get_url() const { return url; }

void NDISource::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("get_name"), &NDISource::get_name);
	ClassDB::bind_method(D_METHOD("get_url"), &NDISource::get_url);
}



// NDI FRAME
NDIFrame::NDIFrame() {}

int64_t NDIFrame::get_timestamp() const { return timestamp; }
Ref<Image> NDIFrame::get_image() const { return image; }
double NDIFrame::get_frame_rate() const { return frame_rate; }
Vector2i NDIFrame::get_original_size() const { return original_size; }
	
void NDIFrame::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("get_timestamp"), &NDIFrame::get_timestamp);
	ClassDB::bind_method(D_METHOD("get_image"), &NDIFrame::get_image);
	ClassDB::bind_method(D_METHOD("get_frame_rate"), &NDIFrame::get_frame_rate);
	ClassDB::bind_method(D_METHOD("get_original_size"), &NDIFrame::get_original_size);
}




VideoStreamNDI::VideoStreamNDI()
{
	search_thread.instantiate();
	receive_thread.instantiate();

	set_create_texture(true);
}

VideoStreamNDI::~VideoStreamNDI()
{
	if (search_thread->is_started()) 
		search_thread->wait_to_finish();

	stop_receiving();

	if (finder)
		NDIlib_find_destroy(finder);
}



void VideoStreamNDI::set_create_texture(bool _create) 
{
	if (create_texture == _create) return;

	create_texture = _create;

	if (create_texture)
		texture.instantiate();
	
	emit_signal("create_texture_changed", create_texture);
}

bool VideoStreamNDI::get_create_texture() const 
{
	return create_texture;
}

Ref<Texture2D> VideoStreamNDI::get_texture() const
{
	return texture;
}



void VideoStreamNDI::set_target_size (const Vector2i& _size)
{
	if (target_size == _size) return;

	receive_mutex.lock();
	target_size = _size;
	receive_mutex.unlock();
}

Vector2i VideoStreamNDI::get_target_size() const
{
	return target_size;
}



void VideoStreamNDI::search()
{
	if (is_search_running.is_set())
		return;
	
	search_thread->start(callable_mp(this, &VideoStreamNDI::_search_thread));
}



TypedArray<NDISource> VideoStreamNDI::_search_thread()
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



void VideoStreamNDI::_search_completed()
{
	TypedArray<NDISource> results = search_thread->wait_to_finish();

	emit_signal("search_completed", results);
}



void VideoStreamNDI::start_receiving(Ref<NDISource> _source) {
	UtilityFunctions::print("STARTED: ", receive_thread->is_started());

	if (!_source.is_valid() || _source.is_null() || current_source == _source) 
		return;

	if (is_receive_running.is_set())
		stop_receiving();

	UtilityFunctions::print("Start receiveing from: ", _source->get_name());

	current_source = _source;
	receive_should_exit.clear();
	receive_thread->start(callable_mp(this, &VideoStreamNDI::_receive_thread));
	UtilityFunctions::print("STARTED: ", receive_thread->is_started());
}



void VideoStreamNDI::_receive_thread()
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

	call_deferred("emit_signal", "receiving_started");


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
						has_valid_frame = false;
						UtilityFunctions::push_warning("Unsupported video color format: ", video.FourCC);
						continue;
				}

				if (!has_valid_frame) continue;

				// CREATE FRAME
				Ref<NDIFrame> frame;
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



void VideoStreamNDI::_frame_received(Ref<NDIFrame> _frame) 
{
	if (!_frame.is_valid() || _frame.is_null()) return;
	
	if (create_texture && _frame->get_image().is_valid()) {
		texture->set_image(_frame->get_image());
	}

	emit_signal("frame_received", _frame);
}



void VideoStreamNDI::stop_receiving() {
	UtilityFunctions::print("STARTED: ", receive_thread->is_started());

	if (!is_receive_running.is_set()) return;

	UtilityFunctions::print("Stop receiving");
	
	receive_should_exit.set();
	receive_thread->wait_to_finish();

	current_source = Ref<NDISource>();

	emit_signal("receiving_stopped");
}



void VideoStreamNDI::_bind_methods()
{
	// METHODS
	ClassDB::bind_method(D_METHOD("search"), &VideoStreamNDI::search);
	ClassDB::bind_method(D_METHOD("start_receiving", "source"), &VideoStreamNDI::start_receiving);
	ClassDB::bind_method(D_METHOD("stop_receiving"), &VideoStreamNDI::stop_receiving);
	
	ClassDB::bind_method(D_METHOD("set_target_size"), &VideoStreamNDI::set_target_size);
	ClassDB::bind_method(D_METHOD("get_target_size"), &VideoStreamNDI::get_target_size);
	
	ClassDB::bind_method(D_METHOD("get_create_texture"), &VideoStreamNDI::get_create_texture);
	ClassDB::bind_method(D_METHOD("set_create_texture"), &VideoStreamNDI::set_create_texture);
	ClassDB::bind_method(D_METHOD("get_texture"), &VideoStreamNDI::get_texture);
	
	ClassDB::bind_method(D_METHOD("_search_completed"), &VideoStreamNDI::_search_completed);
	ClassDB::bind_method(D_METHOD("_frame_received"), &VideoStreamNDI::_frame_received);

	// SIGNALS
	ADD_SIGNAL(MethodInfo("search_completed", PropertyInfo(Variant::ARRAY, "result")));
	ADD_SIGNAL(MethodInfo("receiving_started"));
	ADD_SIGNAL(MethodInfo("receiving_stopped"));
	ADD_SIGNAL(MethodInfo("create_texture_changed", PropertyInfo(Variant::BOOL, "creates")));
	ADD_SIGNAL(MethodInfo("frame_received", PropertyInfo(Variant::OBJECT, "frame")));

	// PROPERTIES
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2I, "target_size", PROPERTY_HINT_NONE, "suffix:px"), "set_target_size", "get_target_size");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "create_texture", PROPERTY_HINT_NONE, ""), "set_create_texture", "get_create_texture");
}


