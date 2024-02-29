#ifndef VIDEO_STREAM_NDI_H
#define VIDEO_STREAM_NDI_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/thread.hpp>
#include <godot_cpp/classes/mutex.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>

#include <godot_cpp/templates/safe_refcount.hpp>

#include <godot_cpp/variant/string.hpp>

#include <Processing.NDI.Lib.h>



using namespace godot;


// comment

class NDISource : public RefCounted
{
	GDCLASS(NDISource, RefCounted);

	friend class VideoStreamNDI;

	public:
		NDISource();

		NDIlib_source_t get_handle() const;
		String get_name() const;
		String get_url() const;
	

	protected:
		static void _bind_methods();
	

	private:
		NDIlib_source_t handle;
		String name;
		String url;
};



class NDIFrame : public RefCounted
{
	GDCLASS(NDIFrame, RefCounted);
	
	friend class VideoStreamNDI;

	public:

		NDIFrame();

		int64_t get_timestamp() const;
		Ref<Image> get_image() const;
		double get_frame_rate() const;
		Vector2i get_original_size() const;
	

	protected:
		static void _bind_methods();
	

	private:
		int64_t timestamp;
		Ref<Image> image;
		double frame_rate;
		Vector2i original_size;
};



class VideoStreamNDI : public Node 
{
	GDCLASS(VideoStreamNDI, Node);
	
public:

	VideoStreamNDI();
	~VideoStreamNDI();

	void search();
	void start_receiving(Ref<NDISource> _source);
	void stop_receiving();

	void set_create_texture(bool _create);
	bool get_create_texture() const;
	Ref<Texture2D> get_texture() const;

	void set_target_size(const Vector2i& _size);
	Vector2i get_target_size() const;

	void _search_completed();
	void _frame_received(Ref<NDIFrame> _frame);


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



#endif // VIDEO_STREAM_NDI_H