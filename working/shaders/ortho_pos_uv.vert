#version 120

attribute vec4 vertex_pos;
attribute vec2 vertex_uv;

uniform mat4 view_projection_transform;

void main(void) {
	gl_FrontColor = gl_Color;
	gl_Position = view_projection_transform * vertex_pos;
	gl_TexCoord[0] = gl_TextureMatrix[0] * vec4(vertex_uv, 0, 1);
}
