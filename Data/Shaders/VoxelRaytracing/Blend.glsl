#ifndef BLEND_GLSL
#define BLEND_GLSL

/**
 * Blend colorSrc and colorDst using UNDER operator (front-to-back blending).
 * Returns true if ray should be terminated early.
 */
bool blend(in vec4 colorSrc, inout vec4 colorDst)
{
	colorDst.rgb = colorDst.rgb + (1.0 - colorDst.a) * colorSrc.a * colorSrc.rgb;
	colorDst.a = colorDst.a + (1.0 - colorDst.a) * colorSrc.a;
	return colorDst.a > 0.99;
}

#endif