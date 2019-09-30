#version 330 core

layout(location = 0) in vec3 vertex_position;
layout(location = 1) in vec2 vertex_uv;

uniform mat4 view_projection_transform;
uniform mat4 model_transform;
uniform mat4 texture_transform;

out vec2 texture_coord;

void main() {
	vec4 vec4_vertex_position = vec4(vertex_position, 1);
	texture_coord = vec2(texture_transform * vec4(vertex_uv, 0, 1));

	gl_Position = view_projection_transform * model_transform * vec4_vertex_position;
}
