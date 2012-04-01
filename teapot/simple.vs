#version 120

uniform mat4 projection_matrix;
uniform mat4 view_matrix;
uniform mat4 model_matrix;
uniform mat3 normal_matrix;
uniform vec3 light_direction;
uniform vec3 material_color;

attribute vec3 position;
attribute vec3 normal;
attribute vec2 tex_coord;

varying float _diffuse;
varying vec2 _tex_coord;

void main(void) {
	vec4 p = vec4(position, 1.0);	
	vec3 n = normal_matrix * normal;	
	
	_diffuse = max(dot(n, -light_direction), 0.0);
	_tex_coord = tex_coord;
	
	gl_Position = projection_matrix * view_matrix * model_matrix * p;
}