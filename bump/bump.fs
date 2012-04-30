#version 120
// porting procedural bumps shaders from "Orange Book" written by 3Dlabs

uniform vec3  surface_color;    // = (0.7, 0.6, 0.18)
uniform float bump_density;     // = 16.0
uniform float bump_size;        // = 0.15
uniform float specular_factor;  // = 0.5

varying vec2 tex_coord;
varying vec3 light_direction;
varying vec3 eye_direction;

void main() {
	vec2 c = bump_density * tex_coord;
	vec2 p = fract(c) - vec2(0.5);

	float d, f;
	d = p.x * p.x + p.y * p.y;
	f = 1.0 / sqrt(d + 1.0);

	if (d >= bump_size) { p = vec2(0.0); f = 1.0; }

	vec3 norm_delta = vec3(p.x, p.y, 1.0) * f;
	vec3 lit_color = surface_color * max(dot(norm_delta, light_direction), 0.0);
	vec3 reflect_direction = reflect(light_direction, norm_delta);
    
	float spec = max(dot(eye_direction, reflect_direction), 0.0);
	spec *= specular_factor;
	lit_color = min(lit_color + spec, vec3(1.0));

	gl_FragColor = vec4(lit_color, 1.0);
}
