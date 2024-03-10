#include "ndi_frames.h"



NDIVideoFrame::NDIVideoFrame() {}



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


