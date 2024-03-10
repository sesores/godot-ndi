extends PanelContainer



@export var ndi:NDIInput = null;

@onready var search:BaseButton = %Search
@onready var start:BaseButton = %Start
@onready var stop:BaseButton = %Stop
@onready var item_list:ItemList = %ItemList

var sources:Array[NDISource] = []
var selected_source:NDISource = null



func _ready():
	start.disabled = true
	stop.disabled = true
	
	search.button_up.connect(on_search)
	start.button_up.connect(on_start)
	stop.button_up.connect(on_stop)
	
	item_list.item_selected.connect(on_item_selected)
	
	ndi.search_completed.connect(on_search_completed)
	ndi.search()



func on_search():
	ndi.search()



func on_start():
	if (!item_list.is_anything_selected() or selected_source == null): return
	
	ndi.start_receiving(selected_source)
	
	start.disabled = true
	stop.disabled = false



func on_stop():
	ndi.stop_receiving()
	
	start.disabled = false
	stop.disabled = true



func on_item_selected(_id:int):
	selected_source = sources[_id]
	start.disabled = false



func on_search_completed(_sources:Array):
	print("SEARCH COMPLETED: ")
	
	sources.clear()
	
	item_list.deselect_all()
	item_list.clear()
	
	for src in _sources:
		var source := src as NDISource
		sources.append(source)
		item_list.add_item(source.get_name() + " (" + source.get_url() + ")")


