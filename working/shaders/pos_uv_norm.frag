#version 120

uniform sampler2D sampler0;
uniform vec3 light_position;
uniform vec3 light_attenuation;
uniform vec4 ambient_light;

varying vec3 world_vertex;
varying vec3 world_normal;

void main() {

	// Get direction to light
	vec3 light_direction = light_position - world_vertex;
	float light_distance = length(light_direction);

	// Normalize
	light_direction /= light_distance;

	// Set ambient light
	vec4 light_color = ambient_light;

	// Calculate diffuse color
	vec4 diffuse_light = vec4(1, 1, 1, 1) * max(dot(world_normal, light_direction), 0.0);
	float attenuation = 1.0 / (light_attenuation.x + light_distance * light_attenuation.y + light_distance * light_distance * light_attenuation.z);
	attenuation = clamp(attenuation, 0.0, 1.0);

	// Add lights up
	light_color += diffuse_light * attenuation;

	// Get texture color
	vec4 texture_color = texture2D(sampler0, vec2(gl_TexCoord[0]));

	// Final color
	gl_FragColor = gl_Color * texture_color * light_color;
}
