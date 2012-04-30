#version 120

uniform sampler2D texture2;

varying vec2 tex_coord;

void main(void) {
	vec3 color = texture2D(texture2, tex_coord).rgb;
	gl_FragColor = vec4(color, 1.0);
}
