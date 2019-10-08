#version 330 core

uniform sampler2DArray sampler0;
uniform sampler2D sampler1;
uniform sampler2D sampler2;
uniform sampler2DArray sampler3;
uniform vec4 ambient_light;
uniform vec4 color;
uniform vec2 tile_count;
uniform vec2 tile_offset;
uniform float texture_scale;
uniform float texture_offset;

in vec2 vertex_coord;
in vec2 texture_coord;
out vec4 out_color;

void main() {

	// Convert vertex position to map coordinate
	ivec2 map_coord = ivec2(vertex_coord);

	// Get texture index from map
	int texture_index0 = int(texelFetch(sampler2, map_coord, 0).r * 255);
	int texture_index1 = int(texelFetch(sampler2, map_coord, 0).g * 255);
	int texture_index2 = int(texelFetch(sampler2, map_coord, 0).a * 255);

	if(texture_index0 == 0)
		discard;

	// Get texture coordinate for the tile
	vec2 tile_texture_coord = texture_coord * tile_count;
	vec2 padded_tile_texture_coord = fract(tile_texture_coord) * texture_scale + texture_offset;

	vec4 texture_color_back = texture(sampler0, vec3(tile_texture_coord, texture_index0));
	vec4 texture_color_trans = texture(sampler3, vec3(padded_tile_texture_coord, texture_index2));

	// Blend first two textures
	vec4 texture_color;
	if(texture_color_trans.r > 0) {
		vec4 texture_color_fore = texture(sampler0, vec3(tile_texture_coord, texture_index1));
		texture_color = mix(texture_color_back, texture_color_fore, texture_color_trans.r);
	}
	else
		texture_color = texture_color_back;

	// Add lights
	vec4 light_color = ambient_light + texelFetch(sampler1, ivec2(gl_FragCoord.xy), 0);
	out_color = color * texture_color * light_color;
}
