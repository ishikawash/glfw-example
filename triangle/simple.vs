#version 120

uniform mat4 projection_matrix;
uniform mat4 view_matrix;
uniform mat4 model_matrix;
uniform mat3 normal_matrix;

attribute vec3 position;
attribute vec3 normal;
attribute vec3 color;

varying vec3 _position;
varying vec3 _normal;
varying vec3 _color;

void main(void) {
	vec4 p = vec4(position, 1.0);
	
	_position = vec3(model_matrix * p);
	_normal = vec3(normal_matrix * normal);
	_color = color;
	
	gl_Position = projection_matrix * view_matrix * model_matrix * p;
}