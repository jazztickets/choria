#version 330 core

#define MAX_LIGHTS 50

uniform sampler2D sampler0;
uniform vec4 ambient_light;
uniform vec4 color;

uniform int light_count;
uniform struct light {
	vec3 position;
	vec4 color;
	float radius;
} lights[MAX_LIGHTS];

smooth in vec3 world_position;
in vec3 world_normal;
in vec2 texture_coord;
out vec4 out_color;

void main() {

	// Set ambient light
	vec4 light_color = ambient_light;

	// Calculate diffuse color
	for(int i = 0; i < light_count; i++) {

		// Get direction to light
		vec3 light_direction = lights[i].position - world_position;
		float light_distance = length(light_direction);

		// Calculate attenuation
		float attenuation = 1.0 / (light_distance + 0.1);
		attenuation = clamp(attenuation, 0.0, 1.0);
		float max = lights[i].radius - 0.25;
		float falloff = 1;
		if(light_distance > max)
			attenuation *= clamp((max + falloff - light_distance) / falloff, 0, 1);

		// Add lights up
		light_color += lights[i].color * attenuation;
	}

	// Get texture color
	vec4 texture_color = texture(sampler0, texture_coord);

	// Final color
	out_color = color * texture_color * light_color;
}
