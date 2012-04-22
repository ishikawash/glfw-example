#version 120
// #define TEXTURE_UNIT_0

struct material_t {
	vec3 diffuse;
	vec3 specular;
	float shininess;
};

uniform vec3 light_position;
uniform material_t material;
uniform sampler2D texture0;

varying vec3 position;
varying vec3 normal;
varying vec2 tex_coord;

void main(void) {
	vec3 light_direction = normalize(light_position - position);	
	vec3 _normal = normalize(normal);
	vec3 halfway = normalize(light_direction - position);
	
	float kd = max(dot(_normal, light_direction), 0.0);	
	float ks = sign(kd) * pow(max(dot(_normal, halfway), 0.0), material.shininess);
	
#ifdef TEXTURE_UNIT_0
	vec3 tex_color = texture2D(texture0, tex_coord).rgb;
	vec3 color = kd * tex_color + ks * material.specular;
#else
	vec3 color = kd * material.diffuse + ks * material.specular;
#endif
	
	gl_FragColor = vec4(clamp(color, 0.0, 1.0), 1.0);
	// gl_FragColor = vec4(ks, ks, ks, 1.0);
}


