#version 120

uniform mat4 projection_matrix;
uniform mat4 model_view_matrix;
uniform mat3 normal_matrix;

uniform mat4 texture_projection_matrix;

attribute vec4 vertex_position;
attribute vec3 vertex_normal;
attribute vec2 vertex_tex_coord;

varying vec2 tex0_coord;
varying vec2 tex1_coord;

void main(void) {
	vec4 p = texture_projection_matrix * vertex_position;
	tex0_coord = 0.5 * ( p.xy / p.w ) + 0.5;
	
	tex1_coord = vertex_tex_coord;
	
	gl_Position = projection_matrix * model_view_matrix * vertex_position;
}
