#version 120

uniform vec3 light_direction;

varying vec3 normal;

void main(void) {
	vec3 _normal = normalize(normal);	
	float kd = max(dot(_normal, -light_direction), 0.0);	
	gl_FragColor = vec4(kd, kd, kd, 1.0);
}


