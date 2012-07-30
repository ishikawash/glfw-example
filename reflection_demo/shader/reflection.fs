#version 120

uniform sampler2D texture0;
uniform sampler2D texture1;
uniform float blend_factor;

varying vec2 tex0_coord;
varying vec2 tex1_coord;

void main(void) {
	vec3 tex0_color = texture2D(texture0, tex0_coord).rgb;
	vec3 tex1_color = texture2D(texture1, tex1_coord).rgb;
	
	vec3 color = clamp(blend_factor * tex0_color + tex1_color, 0.0, 1.0);
	
	gl_FragColor = vec4(color, 1.0);
}
