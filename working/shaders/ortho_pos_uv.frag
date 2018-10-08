#version 330 core

uniform sampler2D sampler0;
uniform vec4 color;

in vec2 texture_coord;
out vec4 out_color;

void main() {
	vec4 texture_color = texture(sampler0, texture_coord);
	out_color = color * texture_color;
}
