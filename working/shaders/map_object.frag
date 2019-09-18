#version 330 core

uniform sampler2D sampler0;
uniform sampler2D sampler1;
uniform vec4 ambient_light;
uniform vec4 color;

in vec2 texture_coord;
out vec4 out_color;

void main() {
	vec4 texture_color = texture(sampler0, texture_coord);
	vec4 light_color = ambient_light + texelFetch(sampler1, ivec2(gl_FragCoord.xy), 0);
	out_color = color * texture_color * light_color;
}
