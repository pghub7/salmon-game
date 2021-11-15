#version 330

// Input attributes
in vec3 in_position;
in vec2 in_texcoord;
layout ( location = 4 ) in vec4 in_part_pos;

out vec2 texCoord;
// out vec4 particleColor;

// Application data
// uniform mat3 transform;
uniform mat3 projection;
// uniform vec4 color;
uniform vec2 scale;
uniform float angle;

void main()
{
	// transform
	mat3 mat = mat3(vec3(1.f, 0.f, 0.f), vec3(0.f, 1.f, 0.f), vec3(0.f, 0.f, 1.f));
	// translate
	mat3 T = mat3(vec3(1.f, 0.f, 0.f), vec3(0.f, 1.f, 0.f),vec3(in_part_pos.x, in_part_pos.y, 1.f));
	mat = mat * T;
	// rotate
	float c = cos(angle);
	float s = sin(angle);
	mat3 R = mat3(vec3(c, s, 0.f ),vec3(-s, c, 0.f),vec3(0.f, 0.f, 1.f));
	mat = mat * R;
	// scale
	mat3 S = mat3(vec3(scale.x, 0.f, 0.f),vec3(0.f, scale.y, 0.f),vec3(0.f, 0.f, 1.f));
	mat = mat * S;

    texCoord = in_texcoord;
	// particleColor = color;
	vec3 pos = projection * mat * vec3(in_position.xy, 1.0);
	gl_Position = vec4(pos.xy, in_position.z, 1.0);
}