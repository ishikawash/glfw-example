#version 120

uniform mat4 projection_matrix;
uniform mat4 view_matrix;
uniform mat4 model_matrix;

attribute vec3 vertex_position;
attribute vec3 vertex_normal;
attribute vec2 vertex_tex_coord;

varying vec2 tex_coord;

void main(void) {
	gl_Position = projection_matrix * view_matrix * model_matrix * vec4(vertex_position, 1.0);
	tex_coord = 0.5 * (gl_Position.xy / gl_Position.w) + 0.5;
}