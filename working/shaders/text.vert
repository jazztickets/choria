#version 120

attribute vec4 vertex_pos;
attribute vec2 vertex_uv;

uniform mat4 view_projection_transform;

void main(void) {
	gl_Position = view_projection_transform * vertex_pos;
	gl_FrontColor = gl_Color;
	gl_TexCoord[0].st = vertex_uv;
}
