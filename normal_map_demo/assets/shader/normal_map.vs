#version 120

uniform mat4 projection_matrix;
uniform mat4 model_view_matrix;
uniform mat3 normal_matrix;
uniform float texcoord_scale;
uniform vec4 light_position; // in eye space

attribute vec4 vertex_position;
attribute vec3 vertex_normal;
attribute vec2 vertex_tex_coord;
attribute vec3 vertex_tangent;

varying vec2 tex_coord;
varying vec3 light_dir;
varying vec3 eye_dir;


mat3 tangent_matrix(vec3 t, vec3 b, vec3 n) {
	return mat3(
		t.x, b.x, n.x,
		t.y, b.y, n.y,
		t.z, b.z, n.z
	);
}

void main(void) {
	vec4 position = model_view_matrix * vertex_position;
	
	vec3 n = normalize(normal_matrix * vertex_normal);
	vec3 t = normalize(normal_matrix * vertex_tangent);
	vec3 b = cross(n, t);

	mat3 m = tangent_matrix(t, b, n);
	light_dir = normalize(m * (light_position.xyz - position.xyz));
	eye_dir = normalize(m * position.xyz);

	tex_coord = texcoord_scale * vertex_tex_coord;

	gl_Position = projection_matrix * position;
}
