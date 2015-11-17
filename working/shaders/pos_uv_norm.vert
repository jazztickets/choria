#version 120

attribute vec4 vertex_pos;
attribute vec2 vertex_uv;
attribute vec3 vertex_normal;

varying vec3 world_vertex;
varying vec3 world_normal;

uniform mat4 view_projection_transform;
uniform mat4 model_transform;

void main(void) {
	world_vertex = vec3(model_transform * vertex_pos);
	world_normal = vertex_normal;

	gl_Position = view_projection_transform * model_transform * vertex_pos;
	gl_FrontColor = gl_Color;
	gl_TexCoord[0] = gl_TextureMatrix[0] * vec4(vertex_uv, 0, 1);
}
