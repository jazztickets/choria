#version 120

uniform sampler2D sampler0;

varying vec2 texture_coord;

void main() {
	vec4 texture_color = texture2D(sampler0, texture_coord);
	gl_FragColor = gl_Color * vec4(1, 1, 1, texture_color.w);
}
