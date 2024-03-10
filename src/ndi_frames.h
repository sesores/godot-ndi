#ifndef NDI_FRAMES_H
#define NDI_FRAMES_H



#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/image.hpp>

#include <godot_cpp/templates/safe_refcount.hpp>



using namespace godot;



class NDIFrame : public RefCounted
{
	GDCLASS(NDIFrame, RefCounted);

	public:
		friend class NDIInput;

		NDIFrame() {};

		int64_t get_timecode() const;
		String get_metadata() const;
	

	protected:
		static void _bind_methods();
	

	private:
		int64_t timecode;
		String metadata;

};



class NDIVideoFrame : public NDIFrame
{
	GDCLASS(NDIVideoFrame, NDIFrame);

	public:
		friend class NDIInput;

		NDIVideoFrame() {};

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



class NDIAudioFrame : public NDIFrame
{
	GDCLASS(NDIAudioFrame, NDIFrame);

	public:
		friend class NDIInput;

		NDIAudioFrame() {};

		int64_t get_timestamp() const;
		
		int get_channel_count() const;
		int get_sample_count() const;
		int get_sample_rate() const;

		const PackedFloat32Array& get_samples() const;
	

	protected:
		static void _bind_methods();
	

	private:
		int64_t timestamp = 0;
		
		int channel_count = 0;
		int sample_count = 0;
		int sample_rate = 0;

		PackedFloat32Array samples;
};



class NDIMetaFrame : public NDIFrame
{
	GDCLASS(NDIMetaFrame, NDIFrame);

	public:
		friend class NDIInput;

		NDIMetaFrame() {};

};



#endif // NDI_FRAMES_H