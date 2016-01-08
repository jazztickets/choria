#version 120

uniform sampler2D sampler0;
uniform vec4 ambient_light;

varying vec3 world_vertex;
varying vec3 world_normal;

void main() {

	// Set ambient light
	vec4 light_color = ambient_light;

	// Get texture color
	vec4 texture_color = texture2D(sampler0, vec2(gl_TexCoord[0]));

	// Final color
	gl_FragColor = gl_Color * texture_color * light_color;
}
