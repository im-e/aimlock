#pragma once

#include <cmath>
#include <algorithm>
#include "Engine/Maths/Vec3.h"

class Vec4
{
public:
	union
	{
		float v[4];
		struct {
			float x, y, z, w;
		};
	};

	Vec4() 
		: x(0), y(0), z(0), w(1) {
	}
	
	Vec4(float _x, float _y, float _z = 0.0f, float _w = 1.0f)
		: x(_x), y(_y), z(_z), w(_w) {
	}

	Vec4(const Vec3& v3, float _w = 1.0f)
		: x(v3.x), y(v3.y), z(v3.z), w(_w) {
	}

	Vec4 operator +(const Vec4& o) const { return Vec4(x + o.x, y + o.y, z + o.z, w + o.w); }
	Vec4 operator -(const Vec4& o) const { return Vec4(x - o.x, y - o.y, z - o.z, w - o.w); }
	Vec4 operator *(float s) const { return Vec4(x * s, y * s, z * s, w * s); }

};