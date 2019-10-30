#version 330 core

layout(location = 0) in vec2 vertex_position;

uniform mat4 view_projection_transform;
uniform vec2 tile_count;
uniform vec2 tile_offset;

out vec2 vertex_coord;
out vec2 texture_coord;

void main() {
	vertex_coord = vertex_position * tile_count + tile_offset;
	texture_coord = vertex_position * tile_count;
	gl_Position = view_projection_transform * vec4(vertex_coord, 0, 1);
}
