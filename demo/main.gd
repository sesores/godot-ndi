extends Node3D



@onready var ndi:NDIInput = $NDIInput
@onready var mesh:MeshInstance3D = $MeshInstance3D

@onready var enable_rotation:CheckButton = %EnableRotation

var material:StandardMaterial3D



func _ready():
	material = mesh.get_active_material(0) as StandardMaterial3D
	material.albedo_texture = ndi.get_texture()
	
	ndi.create_texture_changed.connect(on_create_texture_changed)
	
	ndi.video_frame_received.connect(on_video_frame)
	ndi.audio_frame_received.connect(on_audio_frame)
	ndi.meta_frame_received.connect(on_meta_frame)



func _process(_delta):
	if (enable_rotation.button_pressed):
		mesh.rotate_y(_delta * 0.3)



func on_video_frame(_frame:NDIVideoFrame):
	#print("FRAME: ", _frame.get_frame_rate(), ", ", _frame.get_original_size(), ", ", _frame.get_image().get_size())
	pass



func on_audio_frame(_frame:NDIAudioFrame):
	#print("FRAME: ", _frame.get_frame_rate(), ", ", _frame.get_original_size(), ", ", _frame.get_image().get_size())
	pass



func on_meta_frame(_frame:NDIMetaFrame):
	#print("FRAME: ", _frame.get_frame_rate(), ", ", _frame.get_original_size(), ", ", _frame.get_image().get_size())
	pass



func on_create_texture_changed(_creates:bool):
	material.albedo_texture = ndi.get_texture()

