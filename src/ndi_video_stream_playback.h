#ifndef NDI_VIDEO_STREAM_PLAYBACK_H
#define NDI_VIDEO_STREAM_PLAYBACK_H



#include <godot_cpp/classes/ref_counted.hpp>
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



using namespace godot;



class NDIVideoStreamPlayback : public VideoStreamPlayback
{
	GDCLASS(NDIVideoStreamPlayback, VideoStreamPlayback);

public:
	NDIVideoStreamPlayback();
	~NDIVideoStreamPlayback();

	void _stop() override;
	void _play() override;
	bool _is_playing() const override;
	void _set_paused(bool _paused) override;
	bool _is_paused() const override;

	double _get_length() const override;
	double _get_playback_position() const override;
	void _seek(double _time) override;

	Ref<Texture2D> _get_texture() const override;
	
	void _set_audio_track(int32_t _idx) override;
	void _update(double _delta) override;
	int32_t _get_channels() const override;
	int32_t _get_mix_rate() const override;

	void _frame_received(Ref<NDIVideoFrame> _frame);

	void set_create_texture(bool _create);
	bool get_create_texture() const;

	void set_target_size(const Vector2i& _size);
	Vector2i get_target_size() const;

	void set_source(Ref<NDISource> _source);
	Ref<NDISource> get_source() const;


protected:
	static void _bind_methods();


private:
	void _receive_thread();

	Ref<Thread> receive_thread;
	Mutex receive_mutex;
	SafeFlag is_receive_running;
	SafeFlag receive_should_exit;
	Ref<ImageTexture> texture;
	
	bool is_connected = false;

	Ref<NDISource> current_source;

	bool create_texture = false;
	Vector2i target_size = Vector2i(-1, -1);
};



#endif // NDI_VIDEO_STREAM_PLAYBACK_H
