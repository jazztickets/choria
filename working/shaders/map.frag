#version 330 core

uniform sampler2DArray sampler0;
uniform sampler2D sampler1;
uniform sampler2D sampler2;
uniform sampler2DArray sampler3;
uniform vec4 ambient_light;
uniform vec4 color;
uniform vec2 tile_count;
uniform float texture_scale;
uniform float texture_offset;

in vec2 vertex_coord;
in vec2 texture_coord;
out vec4 out_color;

void main() {

	// Convert vertex position to map coordinate
	ivec2 map_coord = ivec2(vertex_coord);

	int texture_index0 = int(texelFetch(sampler1, map_coord, 0).r * 255);
	int texture_index1 = 0;
	int texture_index2 = 0;

	// Get texture coordinate for the tile
	vec2 tile_texture_coord = fract(texture_coord * tile_count) * texture_scale + texture_offset;

	vec4 texture_color_back = texture(sampler0, vec3(tile_texture_coord, texture_index0));
	vec4 texture_color_trans = texture(sampler3, vec3(tile_texture_coord, texture_index2));

	// Blend first two textures
	vec4 texture_color;
	if(false && texture_color_trans.a > 0) {
		vec4 texture_color_fore = texture(sampler0, vec3(tile_texture_coord, texture_index1));
		texture_color = mix(texture_color_back, texture_color_fore, texture_color_trans.a);
	}
	else
		texture_color = texture_color_back;

	// Add lights
	vec4 light_color = ambient_light + texelFetch(sampler2, ivec2(gl_FragCoord.xy), 0);
	out_color = color * texture_color * light_color;
}
