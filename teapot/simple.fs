#version 120

varying vec3 color;

void main(void) {
	gl_FragColor = vec4(color, 1.0);
	// gl_FragColor = vec4(0.5 * normal + 0.5, 1.0);
}
