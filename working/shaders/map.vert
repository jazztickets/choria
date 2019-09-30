#version 330 core

layout(location = 0) in vec3 vertex_position;
layout(location = 1) in vec2 vertex_uv;
layout(location = 2) in float vertex_texture_index0;

uniform mat4 view_projection_transform;
uniform mat4 model_transform;

out vec2 texture_coord;
out float texture_index0;

void main() {
	vec4 vec4_vertex_position = vec4(vertex_position, 1);
	texture_coord = vertex_uv;
	texture_index0 = vertex_texture_index0;

	gl_Position = view_projection_transform * model_transform * vec4_vertex_position;
}
