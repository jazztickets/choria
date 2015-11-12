#version 120

attribute vec4 vertex_pos;

uniform mat4 view_projection_transform;
uniform mat4 model_transform;

void main(void) {
	gl_Position = view_projection_transform * model_transform * vertex_pos;
	gl_FrontColor = gl_Color;
}
