#version 330 core

layout(location = 0) in vec3 vertex_position;
layout(location = 1) in vec2 vertex_uv0;
layout(location = 2) in vec2 vertex_uv1;

uniform mat4 view_projection_transform;
uniform mat4 model_transform;

out vec2 texture_coord0;
out vec2 texture_coord1;

void main() {
	vec4 vec4_vertex_position = vec4(vertex_position, 1);
	texture_coord0 = vertex_uv0;
	texture_coord1 = vertex_uv1;

	gl_Position = view_projection_transform * model_transform * vec4_vertex_position;
}
