#version 130

uniform float time;
uniform sampler2D texture;

vec4 tex_at(vec2 pix)
{
	return texture2D(texture, vec2(pix.x / 800.0, pix.y / 600.0));
}

void main()
{
	vec4 pixel = tex_at(vec2(gl_FragCoord.x + sin(gl_FragCoord.y / 4.0 + time) * 5.0 + cos(gl_FragCoord.y / 8.0 + time * 2.0) * 2, gl_FragCoord.y));
	gl_FragColor = gl_Color * pixel;
}
