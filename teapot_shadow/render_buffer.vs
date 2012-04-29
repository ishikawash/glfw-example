#version 120

uniform mat4 projection_matrix;
uniform mat4 view_matrix;
uniform mat4 model_matrix;
uniform mat3 normal_matrix;

attribute vec3 vertex_position;
attribute vec3 vertex_normal;
attribute vec2 vertex_tex_coord;

varying vec3 position;

void main(void) {
	gl_Position = projection_matrix * view_matrix * model_matrix * vec4(vertex_position, 1.0);
}
