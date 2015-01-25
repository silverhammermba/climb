#version 130

uniform float winw;
uniform float winh;
uniform float time; // running time of game
uniform float start_time; // when the intro ended
uniform sampler2D texture;

//
// Description : Array and textureless GLSL 2D simplex noise function.
//      Author : Ian McEwan, Ashima Arts.
//  Maintainer : ijm
//     Lastmod : 20110822 (ijm)
//     License : Copyright (C) 2011 Ashima Arts. All rights reserved.
//               Distributed under the MIT License. See LICENSE file.
//               https://github.com/ashima/webgl-noise
//

vec3 mod289(vec3 x) {
	return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec2 mod289(vec2 x) {
	return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec3 permute(vec3 x) {
	return mod289(((x*34.0)+1.0)*x);
}

float snoise(vec2 v)
{
	const vec4 C = vec4(
		 0.211324865405187,  // (3.0-sqrt(3.0))/6.0
		 0.366025403784439,  // 0.5*(sqrt(3.0)-1.0)
		-0.577350269189626,  // -1.0 + 2.0 * C.x
		 0.024390243902439   // 1.0 / 41.0
	);
	// First corner
	vec2 i  = floor(v + dot(v, C.yy) );
	vec2 x0 = v -   i + dot(i, C.xx);

	// Other corners
	vec2 i1;
	//i1.x = step( x0.y, x0.x ); // x0.x > x0.y ? 1.0 : 0.0
	//i1.y = 1.0 - i1.x;
	i1 = (x0.x > x0.y) ? vec2(1.0, 0.0) : vec2(0.0, 1.0);
	// x0 = x0 - 0.0 + 0.0 * C.xx ;
	// x1 = x0 - i1 + 1.0 * C.xx ;
	// x2 = x0 - 1.0 + 2.0 * C.xx ;
	vec4 x12 = x0.xyxy + C.xxzz;
	x12.xy -= i1;

	// Permutations
	i = mod289(i); // Avoid truncation effects in permutation
	vec3 p = permute( permute( i.y + vec3(0.0, i1.y, 1.0 ))
			+ i.x + vec3(0.0, i1.x, 1.0 ));

	vec3 m = max(0.5 - vec3(dot(x0,x0), dot(x12.xy,x12.xy), dot(x12.zw,x12.zw)), 0.0);
	m = m*m ;
	m = m*m ;

	// Gradients: 41 points uniformly over a line, mapped onto a diamond.
	// The ring size 17*17 = 289 is close to a multiple of 41 (41*7 = 287)

	vec3 x = 2.0 * fract(p * C.www) - 1.0;
	vec3 h = abs(x) - 0.5;
	vec3 ox = floor(x + 0.5);
	vec3 a0 = x - ox;

	// Normalise gradients implicitly by scaling m
	// Approximation of: m *= inversesqrt( a0*a0 + h*h );
	m *= 1.79284291400159 - 0.85373472095314 * ( a0*a0 + h*h );

	// Compute final noise value at P
	vec3 g;
	g.x  = a0.x  * x0.x  + h.x  * x0.y;
	g.yz = a0.yz * x12.xz + h.yz * x12.yw;
	return 130.0 * dot(m, g);
}

// get texture at a pixel
vec4 tex_at(vec2 pix)
{
	return texture2D(texture, vec2(pix.x / winw, pix.y / winh));
}

// calculate bloom at a pixel with color and size
vec4 bloom(vec2 pix, vec4 color, float glowsize)
{
	int samples = 10;

	vec4 source = tex_at(pix);
	vec4 sum = vec4(0);
	int diff = (samples - 1) / 2;
	vec2 sizeFactor = vec2(1) * glowsize;

	for (int x = -diff; x <= diff; x++)
	{
		for (int y = -diff; y <= diff; y++)
		{
			vec2 offset = vec2(x, y) * sizeFactor;
			sum += tex_at(pix + offset);
		}
	}

	return ((sum / (samples * samples)) + source) * color;
}

// gl_FragCoord is 0,0 in bottom left, 1,1 in top right

void main()
{
	float n1 = snoise(gl_FragCoord.xy * 20.0 / vec2(winw, winh) + vec2(cos(time), sin(time)));
	vec4 lava = vec4(1.0, 0.0, 0.0, 1.0);

	// calculate normal pixels
	vec4 normpix = tex_at(gl_FragCoord.xy);
	// calculate glow
	vec4 glowpix = bloom(gl_FragCoord.xy + vec2(n1), lava, 5.0);

	float t = 1.0;

	// if the intro is over
	if (start_time >= 0.f)
	{
		// darken higher pixels
		normpix /= (1.0 + smoothstep(start_time, start_time + 0.2, time) * 0.3);
		// and lower pixels blend to lava
		t = smoothstep(0.0, 300.0, gl_FragCoord.y);
	}

	vec4 pixel = mix(glowpix, normpix, t);

	gl_FragColor = gl_Color * pixel;
}
