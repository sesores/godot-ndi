
#include "ndi_frames.h"



int64_t NDIFrame::get_timecode() const 
{ 
	return timecode; 
}

String NDIFrame::get_metadata() const 
{ 
	return metadata; 
}
	
void NDIFrame::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("get_timecode"), &NDIFrame::get_timecode);
	ClassDB::bind_method(D_METHOD("get_metadata"), &NDIFrame::get_metadata);
}



int64_t NDIVideoFrame::get_timestamp() const 
{ 
	return timestamp; 
}

Ref<Image> NDIVideoFrame::get_image() const
{ 
	return image; 
}

double NDIVideoFrame::get_frame_rate() const 
{ 
	return frame_rate; 
}

Vector2i NDIVideoFrame::get_original_size() const 
{
	return original_size; 
}
	
void NDIVideoFrame::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("get_timestamp"), &NDIVideoFrame::get_timestamp);
	ClassDB::bind_method(D_METHOD("get_image"), &NDIVideoFrame::get_image);
	ClassDB::bind_method(D_METHOD("get_frame_rate"), &NDIVideoFrame::get_frame_rate);
	ClassDB::bind_method(D_METHOD("get_original_size"), &NDIVideoFrame::get_original_size);
}



int64_t NDIAudioFrame::get_timestamp() const 
{ 
	return timestamp; 
}

int NDIAudioFrame::get_channel_count() const
{
	return channel_count;
}

int NDIAudioFrame::get_sample_count() const
{
	return sample_count;
}

int NDIAudioFrame::get_sample_rate() const
{
	return sample_rate;
}

const PackedFloat32Array& NDIAudioFrame::get_samples() const
{
	return samples;
}

void NDIAudioFrame::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("get_timestamp"), &NDIAudioFrame::get_timestamp);
	ClassDB::bind_method(D_METHOD("get_channel_count"), &NDIAudioFrame::get_channel_count);
	ClassDB::bind_method(D_METHOD("get_sample_count"), &NDIAudioFrame::get_sample_count);
	ClassDB::bind_method(D_METHOD("get_sample_rate"), &NDIAudioFrame::get_sample_rate);
	ClassDB::bind_method(D_METHOD("get_samples"), &NDIAudioFrame::get_samples);
}


