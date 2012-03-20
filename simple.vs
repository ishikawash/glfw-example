#version 120

uniform mat4 projection_matrix;
uniform mat4 view_matrix;
uniform mat4 model_matrix;

attribute vec3 position;
attribute vec3 color;

varying vec4 _color;

void main(void) {
	_color = vec4(color, 1.0);
	gl_Position = projection_matrix * view_matrix * model_matrix * vec4(position, 1.0);
}