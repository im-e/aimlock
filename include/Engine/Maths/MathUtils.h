#pragma once
#include <cmath>
#include "Engine/Maths/Vec4.h"

constexpr float PI = 3.14159265358979f;

class MathUtils
{
public:
	template <typename T>
	static T lerp(const T a, const T b, float t)
	{
		return a * (1.0f - t) + (b * t);
	}
	
	template <typename T>
	static T simpleInterpolateAttribute(T a0, T a1, T a2, float alpha, float beta, float gamma)
	{
		return (a0 * alpha) + (a1 * beta) + (a2 * gamma);
	}

	template <typename T>
	static T perspectiveCorrectInterpolateAttribute(T a0, T a1, T a2, float v0_w, float v1_w, float v2_w,
		float alpha, float beta, float gamma, float frag_w)
	{
		T attrib[3];
		attrib[0] = a0 * (alpha * v0_w);
		attrib[1] = a1 * (beta * v1_w);
		attrib[2] = a2 * (gamma * v2_w);
		return (attrib[0] + attrib[1] + attrib[2]) / frag_w;
	}

	static Vec4 toScreen(Vec4 vertex, float width, float height)
	{
		float screenX = ((vertex.x + 1) / 2) * width;
		float screenY = (1 - (vertex.y + 1) / 2) * height; //flip y
		return Vec4(screenX, screenY, vertex.z, vertex.w);
	}
};