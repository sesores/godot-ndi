#ifndef NDI_SOURCE_H
#define NDI_SOURCE_H



#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/templates/safe_refcount.hpp>
#include <godot_cpp/variant/string.hpp>

#include <Processing.NDI.Lib.h>



using namespace godot;



class NDISource : public RefCounted
{
	GDCLASS(NDISource, RefCounted);

	public:
		friend class NDIInput;

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



#endif // NDI_SOURCE_H