#version 120

attribute vec4 vertex_pos;
attribute vec2 vertex_uv;

uniform mat4 view_projection_transform;
uniform mat4 model_transform;
uniform mat4 texture_transform;

varying vec2 texture_coord;

void main(void) {
	gl_FrontColor = gl_Color;
	gl_Position = view_projection_transform * model_transform * vertex_pos;
	texture_coord = vec2(texture_transform * vec4(vertex_uv, 0, 1));
}
