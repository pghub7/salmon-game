#version 330

// Input attributes
in vec3 in_position;
in vec2 in_texcoord;

out vec2 texCoord;
// out vec4 particleColor;

// Application data
uniform mat3 transform;
uniform mat3 projection;
// uniform vec4 color;

void main()
{
    texCoord = in_texcoord;
	// particleColor = color;
	vec3 pos = projection * transform * vec3(in_position.xy, 1.0);
	gl_Position = vec4(pos.xy, in_position.z, 1.0);
}