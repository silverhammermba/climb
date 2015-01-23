#version 130

uniform sampler2D texture;

vec4 tex_at(vec2 pix)
{
	return texture2D(texture, vec2(pix.x / 800.0, pix.y / 600.0));
}

void main()
{
	vec4 pixel = (
		tex_at(gl_FragCoord.xy + vec2( 1.0,  0.0)) +
		tex_at(gl_FragCoord.xy + vec2(-1.0,  0.0)) +
		tex_at(gl_FragCoord.xy + vec2( 0.0,  1.0)) +
		tex_at(gl_FragCoord.xy + vec2( 0.0, -1.0)) +
		tex_at(gl_FragCoord.xy + vec2( 1.0,  1.0)) +
		tex_at(gl_FragCoord.xy + vec2( 1.0, -1.0)) +
		tex_at(gl_FragCoord.xy + vec2(-1.0, -1.0)) +
		tex_at(gl_FragCoord.xy + vec2(-1.0,  1.0)) +
		tex_at(gl_FragCoord.xy)
	) / 9.0;
	gl_FragColor = gl_Color * pixel;
}
