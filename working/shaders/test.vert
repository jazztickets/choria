#version 330 core

layout(location = 0) in vec2 vertex_position;
layout(location = 1) in vec2 vertex_uv;

uniform mat4 view_projection_transform;
uniform mat4 model_transform;
uniform mat4 texture_transform;

out vec2 texture_coord;

void main() {
	gl_Position = view_projection_transform * model_transform * vec4(vertex_position, 0, 1);
	texture_coord = vec2(texture_transform * vec4(vertex_uv, 0, 1));
}
