#ifndef NDI_FRAMES_H
#define NDI_FRAMES_H



#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/image.hpp>

#include <godot_cpp/templates/safe_refcount.hpp>



using namespace godot;



class NDIVideoFrame : public RefCounted
{
	GDCLASS(NDIVideoFrame, RefCounted);

	public:
		friend class NDIVideoStreamPlayback;

		NDIVideoFrame();

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



#endif // NDI_FRAMES_H