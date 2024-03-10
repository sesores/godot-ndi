
#include "ndi_source.h"



NDISource::NDISource() 
{
}



String NDISource::get_name() const 
{ 
	return name; 
}



String NDISource::get_url() const 
{ 
	return url; 
}



void NDISource::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("get_name"), &NDISource::get_name);
	ClassDB::bind_method(D_METHOD("get_url"), &NDISource::get_url);
}


