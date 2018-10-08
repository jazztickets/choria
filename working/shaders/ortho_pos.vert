#version 330 core

layout(location = 0) in vec2 vertex_position;

uniform mat4 view_projection_transform;
uniform mat4 model_transform;

void main(void) {
	gl_Position = view_projection_transform * model_transform * vec4(vertex_position, 0, 1);
}
