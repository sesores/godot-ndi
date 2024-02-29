extends VideoStreamNDI



func _ready():
	search_completed.connect(on_search_completed)
	set_target_size(Vector2i(512, 512))
	search()



func on_search_completed(_sources:Array):
	print("SEARCH COMPLETED: ")
	
	for src in _sources:
		var source := src as NDISource
		print("    Name: ", source.get_name())
		print("    URL: ", source.get_url())
	
	if (_sources.is_empty()): 
		print("NO SOURCES FOUND!")
		return
	
	start_receiving(_sources.front())
