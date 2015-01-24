#version 130

uniform float winw;
uniform float winh;
uniform float time;
uniform sampler2D texture;

vec4 tex_at(vec2 pix)
{
	return texture2D(texture, vec2(pix.x / winw, pix.y / winh));
}

void main()
{
	vec4 pixel = tex_at(gl_FragCoord.xy);
	gl_FragColor = gl_Color * pixel;
}
