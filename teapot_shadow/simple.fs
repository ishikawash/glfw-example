#version 120
// #define TEXTURE_UNIT_1

struct material_t {
	vec3 diffuse;
	vec3 specular;
	float shininess;
};

const float depth_bias = -0.0005;

uniform vec3 light_position;
uniform material_t material;
uniform sampler2D texture1;
uniform sampler2D texture2;

varying vec3 position;
varying vec3 normal;
varying vec2 tex_coord;
varying vec4 light_coord;

void main(void) {
	vec3 light_direction = normalize(light_position - position);	
	vec3 _normal = normalize(normal);
	vec3 halfway = normalize(light_direction - position);
	
	float kd = max(dot(_normal, light_direction), 0.0);	
	float ks = sign(kd) * pow(max(dot(_normal, halfway), 0.0), material.shininess);
	
#ifdef TEXTURE_UNIT_1
	vec3 tex_color = texture2D(texture1, tex_coord).rgb;
	vec3 color = kd * tex_color + ks * material.specular;
#else
	vec3 color = kd * material.diffuse + ks * material.specular;
#endif

	vec4 _light_coord = light_coord / light_coord.w;
	_light_coord.z += depth_bias;
	float distance_from_light = texture2D(texture2, _light_coord.xy).z;
	float shadow = 1.0;
	if (light_coord.w > 0.0) {
		shadow = distance_from_light < _light_coord.z ? 0.5 : 1.0;
	}	

	gl_FragColor = vec4(clamp(shadow * color, 0.0, 1.0), 1.0);
	// gl_FragColor = vec4(shadow, shadow, shadow, 1.0);
}


