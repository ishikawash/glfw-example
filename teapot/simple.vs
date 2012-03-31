#version 120

uniform mat4 projection_matrix;
uniform mat4 view_matrix;
uniform mat4 model_matrix;
uniform mat3 normal_matrix;
uniform vec3 light_direction;
uniform vec3 material_color;

attribute vec3 position;
attribute vec3 normal;

varying vec3 color;

void main(void) {
	vec4 p = vec4(position, 1.0);	
	vec3 n = normal_matrix * normal;	
	float kd = max(dot(n, -light_direction), 0.0);
	color = kd * material_color;
	
	gl_Position = projection_matrix * view_matrix * model_matrix * p;
}