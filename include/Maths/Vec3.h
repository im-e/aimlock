#pragma once
#include <cmath>
#include <algorithm>

class Vec3
{
public:
	union
	{
		float v[3];
		struct {
			float x, y, z;
		};
	};

	Vec3()
		: x(0), y(0), z(0) {
	}

	Vec3(float _x, float _y, float _z = 0.0f)
		: x(_x), y(_y), z(_z) {
	}

	Vec3 operator +(const Vec3& pVec) const
	{
		return Vec3(v[0] + pVec.v[0], v[1] + pVec.v[1], v[2] + pVec.v[2]);
	}
	Vec3& operator +=(const Vec3& pVec)
	{
		v[0] += pVec.v[0];
		v[1] += pVec.v[1];
		v[2] += pVec.v[2];
		return *this;
	}

	Vec3 operator -(const Vec3& pVec) const
	{
		return Vec3(v[0] - pVec.v[0], v[1] - pVec.v[1], v[2] - pVec.v[2]);
	}

	Vec3 operator /(const float val) const
	{
		float inv = 1.0f / val;
		return Vec3(v[0] * inv, v[1] * inv, v[2] * inv);
	}

	Vec3 operator *(const Vec3& pVec) const
	{
		return Vec3(v[0] * pVec.v[0], v[1] * pVec.v[1], v[2] * pVec.v[2]);
	}
	Vec3 operator *(const float val) const
	{
		return Vec3(v[0] * val, v[1] * val, v[2] * val);
	}

	Vec3 operator - () const
	{
		return Vec3(-v[0], -v[1], -v[2]);
	}

	Vec3 normalize()
	{
		float len = 1.0f / sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
		return Vec3(x * len, y * len, z * len);
	}

	float normalize_GetLength()
	{
		float length = sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
		float len = 1.0f / length;
		v[0] *= len; v[1] *= len; v[2] *= len;
		return length;
	}

	float Dot(const Vec3& pVec) const
	{
		return v[0] * pVec.v[0] + v[1] * pVec.v[1] + v[2] * pVec.v[2];
	}

	Vec3 Cross(const Vec3& v1)
	{
		return Vec3(v1.v[1] * v[2] - v1.v[2] * v[1],
			v1.v[2] * v[0] - v1.v[0] * v[2],
			v1.v[0] * v[1] - v1.v[1] * v[0]);
	}

	float Max() const
	{
		return std::max(x, std::max(y, z));
	}

};


inline float Dot(const Vec3& a, const Vec3& b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline Vec3 Cross(const Vec3& a, const Vec3& b)
{
	return Vec3(a.y * b.z - a.z * b.y,
		a.z * b.x - a.x * b.z,
		a.x * b.y - a.y * b.x);
}

