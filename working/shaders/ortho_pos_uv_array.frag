#version 330 core

uniform sampler2DArray sampler;
uniform vec4 color;
uniform float texture_index;

in vec2 texture_coord;
out vec4 out_color;

void main() {
	vec4 texture_color = texture(sampler, vec3(texture_coord, texture_index));
	out_color = color * texture_color;
}
