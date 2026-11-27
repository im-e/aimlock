#pragma once
#include <cmath>
#include "Engine/Maths/Vec3.h"

constexpr float PI = 3.14159265358979f;

class MathUtils
{
public:
	template <typename T>
	static T lerp(const T a, const T b, float t)
	{
		return a * (1.0f - t) + (b * t);
	}

	static float toRadians(float degrees) { return degrees * PI / 180.0f; }
	static float toDegrees(float radians) { return radians * 180.0f / PI; }

	//ray vs sphere, used for hitscan shooting
	//dir has to be normalised. if it hits, t is how far along the ray the hit is
	static bool raySphere(const Vec3& origin, const Vec3& dir, const Vec3& centre, float radius, float& t)
	{
		//solve |origin + dir * t - centre| = radius, which is a quadratic in t
		//with dir normalised the 'a' term is 1, so this is the simplified version
		Vec3 oc = origin - centre;
		float b = Dot(oc, dir);
		float c = Dot(oc, oc) - radius * radius;
		float discriminant = b * b - c;
		if (discriminant < 0.0f)
			return false; //missed completely

		float root = sqrtf(discriminant);
		t = -b - root;     //nearest hit
		if (t < 0.0f)
			t = -b + root; //we're inside the sphere, so take the far side
		return t >= 0.0f;
	}
};
