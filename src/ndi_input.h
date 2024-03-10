#ifndef NDI_INPUT_H
#define NDI_INPUT_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/node.hpp>
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




class NDIInput : public Node 
{
	GDCLASS(NDIInput, Node);
	
public:

	NDIInput();
	~NDIInput();

	void search();
	void start_receiving(Ref<NDISource> _source);
	void stop_receiving();

	void set_create_texture(bool _create);
	bool get_create_texture() const;
	Ref<Texture2D> get_texture() const;

	void set_target_size(const Vector2i& _size);
	Vector2i get_target_size() const;

	void _search_completed();

	void _video_frame_received(Ref<NDIVideoFrame> _frame);
	void _audio_frame_received(Ref<NDIAudioFrame> _frame);
	void _meta_frame_received(Ref<NDIMetaFrame> _frame);


protected:
	static void _bind_methods();


private:
	TypedArray<NDISource> _search_thread();
	void _receive_thread();

	Ref<Thread> search_thread;
	SafeFlag is_search_running;
	NDIlib_find_instance_t finder = nullptr;

	Ref<Thread> receive_thread;
	Mutex receive_mutex;
	SafeFlag is_receive_running;
	SafeFlag receive_should_exit;

	Ref<NDISource> current_source;

	Ref<ImageTexture> texture;
	bool create_texture = false;
	Vector2i target_size = Vector2i(-1, -1);

};



#endif // NDI_INPUT_H