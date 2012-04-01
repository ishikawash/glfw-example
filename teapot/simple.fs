#version 120

uniform sampler2D texture0;

varying float _diffuse;
varying vec2 _tex_coord;

void main(void) {
	// gl_FragColor = vec4(color, 1.0);
	vec3 color = _diffuse * texture2D(texture0, _tex_coord).rgb;
	gl_FragColor = vec4(color, 1.0);
}
