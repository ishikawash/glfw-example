#version 120

uniform mat4 projection_matrix;
uniform mat4 view_matrix;
uniform mat4 model_matrix;
uniform mat3 normal_matrix;
uniform vec3 material_color;

attribute vec3 position;
attribute vec3 normal;

varying vec3 _position;
varying vec3 _normal;


void main(void) {
	vec4 p = model_matrix * vec4(position, 1.0);	
	_position = vec3(p);
	_normal = normal_matrix * normal;	
		
	gl_Position = projection_matrix * view_matrix * p;
}