#version 330

// From vertex shader
in vec2 texCoord;
// in vec4 particleColor;

// Application data
uniform sampler2D particleTexture;
uniform vec4 deathParticleColor;

// Output color
layout(location = 0) out  vec4 color;

void main()
{
	color = (texture(particleTexture, texCoord) * deathParticleColor);
}