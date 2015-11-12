#version 120

uniform sampler2D sampler0;

void main() {
	vec4 texture_color = texture2D(sampler0, vec2(gl_TexCoord[0]));
	gl_FragColor = gl_Color * vec4(1, 1, 1, texture_color.w);
}
