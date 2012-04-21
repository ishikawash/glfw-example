#version 120

uniform mat4 projection_matrix;
uniform mat4 view_matrix;
uniform mat4 model_matrix;
uniform mat3 normal_matrix;
uniform vec3 material_color;

attribute vec3 vertex_position;
attribute vec3 vertex_normal;

varying vec3 position;
varying vec3 normal;


void main(void) {
	vec4 p = model_matrix * vec4(vertex_position, 1.0);	
	position = vec3(p);
	normal = normal_matrix * vertex_normal;	
		
	gl_Position = projection_matrix * view_matrix * p;
}