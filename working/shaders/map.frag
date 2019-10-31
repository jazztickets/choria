#version 330 core

uniform sampler2DArray sampler0;
uniform sampler2D sampler1;
uniform sampler2DArray sampler2;
uniform sampler2DArray sampler3;
uniform vec4 ambient_light;
uniform vec4 color;
uniform float texture_scale;
uniform float texture_offset;

in vec2 vertex_coord;
in vec2 texture_coord;
out vec4 out_color;

// Mix two textures with a transition layer in-between
vec4 mix_layers(vec2 layer_coord, vec2 trans_coord, vec4 bottom_color, int trans_index, int top_index) {

	// No transition
	if(trans_index == 0)
		return bottom_color;

	// Get transition texture
	vec4 trans_color = texture(sampler3, vec3(trans_coord, trans_index));

	// Check for transparency
	if(trans_color.r == 0)
		return bottom_color;

	// Blend colors
	vec4 top_color = texture(sampler0, vec3(layer_coord, top_index));
	vec4 texture_color = mix(bottom_color, top_color, trans_color.r);

	return texture_color;
}

void main() {

	// Get texture coordinates for padded transition tiles
	vec2 padded_texture_coord = fract(texture_coord) * texture_scale + texture_offset;

	// Convert vertex position to map coordinate
	ivec3 map_coord0 = ivec3(vertex_coord, 0);
	ivec3 map_coord1 = ivec3(vertex_coord, 1);

	// Get base index
	int base_index = int(texelFetch(sampler2, map_coord0, 0).r * 255);
	if(base_index == 0)
		discard;

	// Get texture indexes
	int first_trans_index = int(texelFetch(sampler2, map_coord0, 0).g * 255);
	int first_layer_index = int(texelFetch(sampler2, map_coord0, 0).b * 255);
	int second_trans_index = int(texelFetch(sampler2, map_coord0, 0).a * 255);
	int second_layer_index = int(texelFetch(sampler2, map_coord1, 0).r * 255);
	int third_trans_index = int(texelFetch(sampler2, map_coord1, 0).g * 255);
	int third_layer_index = int(texelFetch(sampler2, map_coord1, 0).b * 255);
	int foreground_index = int(texelFetch(sampler2, map_coord1, 0).a * 255);

	// Mix textures
	vec4 texture_color = texture(sampler0, vec3(texture_coord, base_index));
	texture_color = mix_layers(texture_coord, padded_texture_coord, texture_color, first_trans_index, first_layer_index);
	texture_color = mix_layers(texture_coord, padded_texture_coord, texture_color, second_trans_index, second_layer_index);
	texture_color = mix_layers(texture_coord, padded_texture_coord, texture_color, third_trans_index, third_layer_index);

	// Add foreground layer
	if(foreground_index != 0) {
		vec4 foreground_color = texture(sampler0, vec3(texture_coord, foreground_index));
		texture_color = mix(texture_color, foreground_color, foreground_color.a);
	}

	// Add lights
	vec4 light_color = ambient_light + texelFetch(sampler1, ivec2(gl_FragCoord.xy), 0);

	// Add emissive component from alpha channel of texture
	light_color += (1.0f - texture_color.a) * vec4(1.0f);

	// Get final color
	out_color = color * texture_color * light_color;
}
