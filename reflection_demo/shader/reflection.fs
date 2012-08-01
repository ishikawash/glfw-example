#version 120

uniform sampler2D texture0;
uniform sampler2D texture1;
uniform float blend_factor;
uniform ivec2 viewport;

varying vec2 tex_coord;

void main(void) {
	vec3 tex0_color = texture2D(texture0, gl_FragCoord.xy / viewport).rgb;
	vec3 tex1_color = texture2D(texture1, tex_coord).rgb;
	
	vec3 color = clamp(blend_factor * tex0_color + tex1_color, 0.0, 1.0);
	
	gl_FragColor = vec4(color, 1.0);
}
