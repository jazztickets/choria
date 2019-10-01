#version 330 core

uniform sampler2DArray sampler0;
uniform sampler2D sampler1;
uniform sampler2DArray sampler2;
uniform vec4 ambient_light;
uniform vec4 color;

in vec2 texture_coord;
in float texture_index0;
in float texture_index1;
in float texture_index2;
in float texture_index3;
out vec4 out_color;

void main() {
	vec4 texture_color_back = texture(sampler0, vec3(texture_coord, texture_index0));
	vec4 texture_color_fore = texture(sampler0, vec3(texture_coord, texture_index1));
	vec4 texture_color_trans = texture(sampler2, vec3(texture_coord, texture_index2));
	vec4 texture_color_corner = texture(sampler2, vec3(texture_coord, texture_index3));

	float trans = max(texture_color_trans.a, texture_color_corner.a);

	// Blend first two textures
	vec4 texture_color;
	if(trans > 0)
		texture_color = mix(texture_color_back, texture_color_fore, trans);
	else
		texture_color = texture_color_back;

	// Add lights
	vec4 light_color = ambient_light + texelFetch(sampler1, ivec2(gl_FragCoord.xy), 0);
	out_color = color * texture_color * light_color;
}
