#version 120
// #define TEXTURE_UNIT_1

struct material_t {
	vec3 diffuse;
	vec3 specular;
	float shininess;
};

const float depth_bias = -0.0005;

uniform material_t material;
uniform sampler2D texture1;
uniform sampler2D texture2;

varying vec3 position;
varying vec3 normal;
varying vec2 tex_coord;
varying vec4 light_coord;
varying vec3 light_position;

void main(void) {
	vec3 eye_dir = normalize(-position);
	vec3 light_dir = normalize(light_position - position);	
	vec3 _normal = normalize(normal);
	
	vec3 reflection_dir = normalize(light_dir + eye_dir);
	// vec3 reflection_dir = normalize(-reflect(light_dir, _normal));
	
	float kd = max(dot(_normal, light_dir), 0.0);	
	float ks = sign(kd) * pow(max(dot(_normal, reflection_dir), 0.0), material.shininess);
	
#ifdef TEXTURE_UNIT_1
	vec3 tex_color = texture2D(texture1, tex_coord).rgb;
	vec3 color = kd * tex_color + ks * material.specular;
#else
	vec3 color = kd * material.diffuse + ks * material.specular;
#endif

	vec4 light_coord_normalized = light_coord / light_coord.w;
	light_coord_normalized.z += depth_bias; // add bias to avoid depth fighting
	float distance_from_light = texture2D(texture2, light_coord_normalized.xy).z;
	float shadow_factor = ( (light_coord.w > 0.0) && (distance_from_light < light_coord_normalized.z) ) ? 0.5 : 1.0;
	
	gl_FragColor = vec4(clamp(shadow_factor * color, 0.0, 1.0), 1.0);
	// gl_FragColor = vec4(shadow, shadow, shadow, 1.0);
}


