#version 120

uniform mat4 projection_matrix;
uniform mat4 model_view_matrix;
uniform mat4 view_matrix;
uniform mat4 view_inverse_matrix;
uniform mat3 normal_matrix;
uniform mat4 light_pov_matrix;
uniform vec3 light_world_position;

attribute vec3 vertex_position;
attribute vec3 vertex_normal;
attribute vec2 vertex_tex_coord;
attribute vec3 vertex_tangent;

varying vec3 position;
varying vec3 normal;
varying vec2 tex_coord;
varying vec4 light_coord;
varying vec3 light_position;

void main(void) {
	vec4 v = model_view_matrix * vec4(vertex_position, 1.0);	
	
	position = v.xyz;
	normal = normal_matrix * vertex_normal;	
	tex_coord = vertex_tex_coord;
	light_coord = light_pov_matrix * view_inverse_matrix * v;
	light_position = vec3(view_matrix * vec4(light_world_position, 1.0));
	
	gl_Position = projection_matrix * v;
}