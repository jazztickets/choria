#version 330 core

uniform sampler2D sampler0;
uniform vec4 ambient_light;
uniform vec4 color;

in vec2 texture_coord;
out vec4 out_color;

void main() {

	// Set ambient light
	vec4 light_color = ambient_light;

	// Get texture color
	vec4 texture_color = texture(sampler0, texture_coord);

	// Final color
	out_color = color * texture_color * light_color;
}
