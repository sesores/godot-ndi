#ifndef NDI_VIDEO_STREAM_H
#define NDI_VIDEO_STREAM_H



#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/video_stream.hpp>
#include <godot_cpp/classes/video_stream_playback.hpp>
#include <godot_cpp/classes/thread.hpp>
#include <godot_cpp/classes/mutex.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>

#include <godot_cpp/templates/safe_refcount.hpp>

#include <godot_cpp/variant/string.hpp>

#include <Processing.NDI.Lib.h>

#include "ndi_source.h"
#include "ndi_frames.h"
#include "ndi_video_stream_playback.h"



using namespace godot;



class NDIVideoStream : public VideoStream 
{
	GDCLASS(NDIVideoStream, VideoStream);
	
public:
	NDIVideoStream();
	~NDIVideoStream();

	Ref<VideoStreamPlayback> _instantiate_playback() override { return Ref<NDIVideoStreamPlayback>(); };

	void search();
	void _search_completed();


protected:
	static void _bind_methods();


private:
	TypedArray<NDISource> _search_thread();

	Ref<Thread> search_thread;
	SafeFlag is_search_running;
	NDIlib_find_instance_t finder = nullptr;
};



#endif // NDI_VIDEO_STREAM_H