#version 130
in vec4 pos_wc;
in vec3 norm_wc;
in vec2 tc;
out vec4 out_col;
out vec4 out_spec;
out vec3 out_pos;
out vec3 out_norm;
out vec2 out_tc; // only for nr
uniform sampler2D diffuse;
uniform sampler2D normalmap;
uniform sampler2D specular; 

vec3 align(in vec3 axis, in vec3 v) {
    float s = sign(axis.z + 0.001f);
    vec3 w = vec3(v.x, v.y, v.z * s);
    vec3 h = vec3(axis.x, axis.y, axis.z + s);
    float k = dot(w, h) / (1.0f + abs(axis.z));
    return k * h - w;
}

void main() {
	vec3 N = align(norm_wc, 2 * texture(normalmap, tc).xyz - 1);
    out_norm = N;

    vec4 diff = texture(diffuse, tc);
    out_spec = texture(specular, tc);
    out_pos = pos_wc.xyz;

    // only for nr
    out_tc = tc;

    out_col = vec4((out_pos+vec3(10))/20,1); //diff;//vec4((out_pos+vec3(10))/20,1);
}
