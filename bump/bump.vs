#version 120
// porting procedural bumps shaders from "Orange Book" written by 3Dlabs

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

varying vec2 tex_coord;
varying vec3 light_direction;
varying vec3 eye_direction;

void main(void) {
	vec4 v = model_view_matrix * vec4(vertex_position, 1.0);
	vec3 n = normalize(normal_matrix * vertex_normal);
	vec3 t = normalize(normal_matrix * vertex_tangent);
	vec3 b = cross(t, n);

	float x, y, z;
	x = dot(light_world_position, t);
	y = dot(light_world_position, b);
	z = dot(light_world_position, n);
	light_direction = normalize(vec3(x, y, z));
	vec3 _v = v.xyz;
	x = dot(_v, t);
	y = dot(_v, b);
	z = dot(_v, n);
	eye_direction = normalize(vec3(x, y, z));
	
	tex_coord = vertex_tex_coord;
	gl_Position = projection_matrix * v;
}
