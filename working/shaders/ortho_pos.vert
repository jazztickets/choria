#version 120

attribute vec4 vertex_pos;

uniform mat4 view_projection_transform;

void main(void) {
	gl_FrontColor = gl_Color;
	gl_Position = view_projection_transform * vertex_pos;
}
