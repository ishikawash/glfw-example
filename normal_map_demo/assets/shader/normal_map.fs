#version 120

uniform sampler2D texture1; // image texture
uniform sampler2D texture2; // normal texture
uniform sampler2D texture3; // height texture

uniform vec2 scale_bias;
uniform vec3 specular_color;
uniform float specular_power;

varying vec2 tex_coord;
varying vec3 light_dir;
varying vec3 eye_dir;

void main(void) {
	vec3 l = normalize(light_dir);
	vec3 e = normalize(-eye_dir);
	
	float height = texture2D(texture3, tex_coord).r;
	float offset = height * scale_bias.r + scale_bias.g;
	vec2 tex_coord_parallax = tex_coord + offset*e.xy;
	
	vec3 n = 2.0 * texture2D(texture2, tex_coord_parallax).rgb - 1.0;
	vec3 h = normalize(l + e);
	
	float kd = clamp(dot(l, n), 0.0, 1.0);
	float ks = sign(kd) * pow(clamp(dot(h, n), 0.0, 1.0), specular_power);
	
	vec3 base_color = texture2D(texture1, tex_coord_parallax).rgb;	
	vec3 frag_color = clamp(kd * base_color + ks * specular_color, 0.0, 1.0);
		
	gl_FragColor = vec4(frag_color, 1.0);
	// gl_FragColor = vec4(vec3(height), 1.0);
}
