extends Node3D



@onready var ndi:NDIInput = $NDIInput
@onready var audio_player:AudioStreamPlayer = $AudioStreamPlayer
@onready var mesh:MeshInstance3D = $MeshInstance3D

@onready var enable_rotation:CheckButton = %EnableRotation

var material:StandardMaterial3D

var samples := PackedFloat32Array()
var audio_playback:AudioStreamGeneratorPlayback = null
var sample_hz:float = 440.0
var pulse_hz:float = 440.0




func _ready():
	sample_hz = audio_player.stream.mix_rate
	
	audio_player.play()
	audio_playback = audio_player.get_stream_playback()
	fill_audio_buffer()
	
	material = mesh.get_active_material(0) as StandardMaterial3D
	material.albedo_texture = ndi.get_texture()
	
	ndi.create_texture_changed.connect(on_create_texture_changed)
	
	ndi.video_frame_received.connect(on_video_frame)
	ndi.audio_frame_received.connect(on_audio_frame)
	ndi.meta_frame_received.connect(on_meta_frame)



func _process(_delta):
	# AUDIO
	#var frames_available = audio_playback.get_frames_available()
	#print("AUDIO: ", frames_available)
	#for i in range(frames_available):
		#audio_playback.push_frame(Vector2.ZERO)
	
	# ROTATION
	if (enable_rotation.button_pressed):
		mesh.rotate_y(_delta * 0.3)



func on_video_frame(_frame:NDIVideoFrame):
	#print("FRAME: ", _frame.get_frame_rate(), ", ", _frame.get_original_size(), ", ", _frame.get_image().get_size())
	pass



func on_audio_frame(_frame:NDIAudioFrame):
	#samples.append_array(_frame.get_samples())
	
	var available = audio_playback.get_frames_available()
	var samples:PackedFloat32Array = _frame.get_samples()
	
	var count:int = mini(_frame.get_sample_count() / _frame.get_channel_count(), available)
	
	print("AUDIO FRAME: ", available, " x ", _frame.get_sample_count())
	
	for i in range(count):
		audio_playback.push_frame(Vector2(samples[i], 0.0))



func fill_audio_buffer():
	var phase = 0.0
	var increment = pulse_hz / sample_hz
	var frames_available = audio_playback.get_frames_available()
	
	for i in range(frames_available):
		audio_playback.push_frame(Vector2.ONE * sin(phase * TAU))
		phase = fmod(phase + increment, 1.0)


func on_meta_frame(_frame:NDIMetaFrame):
	#print("FRAME: ", _frame.get_frame_rate(), ", ", _frame.get_original_size(), ", ", _frame.get_image().get_size())
	pass



func on_create_texture_changed(_creates:bool):
	material.albedo_texture = ndi.get_texture()

