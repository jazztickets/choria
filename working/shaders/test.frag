#version 330 core

uniform sampler2DArray sampler0;
uniform sampler2D sampler1;
uniform sampler2DArray sampler2;
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

	// Get texture coordinate for the tile
	vec2 tile_texture_coord = fract(texture_coord * tile_count) * texture_scale + texture_offset;

	// Get base texture color
	vec4 texture_color_back = texture(sampler0, vec3(tile_texture_coord, (map_coord.x + map_coord.y) % 3 + 1));

	out_color = texture_color_back * color;
}
