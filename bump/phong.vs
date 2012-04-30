#version 120

uniform mat4 projection_matrix;
uniform mat4 view_matrix;
uniform mat4 model_matrix;
uniform mat3 normal_matrix;
uniform mat4 light_pov_matrix;

attribute vec3 vertex_position;
attribute vec3 vertex_normal;
attribute vec2 vertex_tex_coord;
attribute vec3 vertex_tangent;

varying vec3 position;
varying vec3 normal;
varying vec2 tex_coord;
varying vec4 light_coord;

void main(void) {
	vec4 v = model_matrix * vec4(vertex_position, 1.0);	
	
	position = v.xyz;
	normal = normal_matrix * vertex_normal;	
	tex_coord = vertex_tex_coord;
	light_coord = light_pov_matrix * v;
	
	gl_Position = projection_matrix * view_matrix * v;
}