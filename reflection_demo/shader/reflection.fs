#version 120

uniform sampler2D texture0;
uniform sampler2D texture1;
uniform float R0;
uniform ivec2 viewport;

varying vec3 position;
varying vec3 normal;
varying vec2 tex_coord;

void main(void) {	
	vec3 i = normalize(position);
	vec3 n = normalize(normal);
	float ratio = R0 + (1.0 - R0) * pow(1.0 - dot(n, -i), 5.0);
	
	vec3 reflection_color = texture2D(texture0, gl_FragCoord.xy / viewport).rgb;
	vec3 refraction_color = texture2D(texture1, tex_coord).rgb;	
	vec3 color = mix(refraction_color, reflection_color, ratio);
		
	gl_FragColor = vec4(color, 1.0);
}
