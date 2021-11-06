#version 330

uniform sampler2D screen_texture;
uniform float time;
uniform float darken_screen_factor;
uniform int basic_mode;
in vec2 texcoord;

layout(location = 0) out vec4 color;

vec2 distort(vec2 uv) 
{
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// TODO A1: HANDLE THE WATER WAVE DISTORTION HERE (you may want to try sin/cos)
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// uv.x=( 0.1*uv.x)+0.1;
    // uv.y=(-0.1*uv.y)+0.1;
	// uv.y = (( uv.y + sin(uv.x * 0.1 ) * 0.15 * sin(time/2.5) ) + 1 )/2.0;
	// uv.x += sin(uv.y * 0.1 ) * 0.06 * sin(time/3) * 0.5 + 0.5;
	// uv.x = uv.x + sin(uv.y * 0.01 + time/2) * 0.01;
	// uv.y += sin(uv.x * 0.01 + time/2) * 0.01;
	
	float x_offset =  sin(uv.y * 0.01 + time/2) * 0.01;
	float y_offset =  sin(uv.x * 0.01 + time/2) * 0.01;
	if (uv.y + y_offset <= 0) {
		return uv;
	}

	return vec2(uv.x, uv.y + y_offset);
}

vec4 color_shift(vec4 in_color) 
{
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// TODO A1: HANDLE THE COLOR SHIFTING HERE (you may want to make it blue-ish)
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	if (basic_mode == 1) {
		vec3 mixedCol = mix( vec3( 0.0, 0.3,  1.0), in_color.rgb, 0.5);
		return vec4(mixedCol, 0);
	}
	if (basic_mode == 0) {
		vec3 mixedCol = mix( vec3( 1.0, 0.7,  0.0), in_color.rgb, 0.5 );
		return vec4(mixedCol, 0);
	 }
	 return in_color;
}

vec4 fade_color(vec4 in_color) 
{
	vec3 c = in_color.xyz;

	return vec4(c, 0.1);
}

void main()
{
	vec2 coord = distort(texcoord);

    vec4 in_color = texture(screen_texture, coord);
    color = color_shift(in_color);
	// color = in_color;
    color = fade_color(color);
}