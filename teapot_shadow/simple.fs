#version 120

struct material_t {
	vec3 diffuse;
	vec3 specular;
	float shininess;
};

uniform vec3 light_position;
uniform material_t material;

varying vec3 position;
varying vec3 normal;

void main(void) {
	vec3 light_direction = normalize(light_position - position);	
	vec3 _normal = normalize(normal);
	vec3 halfway = normalize(light_direction - position);
	
	float kd = max(dot(_normal, light_direction), 0.0);	
	float ks = sign(kd) * pow(max(dot(_normal, halfway), 0.0), material.shininess);
	
	vec3 color = clamp(kd * material.diffuse + ks * material.specular, 0.0, 1.0);
	gl_FragColor = vec4(color, 1.0);
	// gl_FragColor = vec4(ks, ks, ks, 1.0);
}
