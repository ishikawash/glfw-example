#version 120

uniform vec3 light_position;

varying vec3 _position;
varying vec3 _normal;

void main(void) {
	vec3 light_direction = normalize(light_position - _position);	
	vec3 normal = normalize(_normal);
	float kd = max(dot(normal, light_direction), 0.0);
	gl_FragColor = vec4(kd, kd, kd, 1.0);
}
