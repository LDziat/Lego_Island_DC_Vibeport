#pragma once

#include "miniwin/d3drm.h"

#include <math.h>

typedef D3DVALUE Matrix3x3[3][3];

inline D3DVECTOR TransformNormal(const D3DVECTOR& v, const Matrix3x3& m)
{
	return {
		v.x * m[0][0] + v.y * m[1][0] + v.z * m[2][0],
		v.x * m[0][1] + v.y * m[1][1] + v.z * m[2][1],
		v.x * m[0][2] + v.y * m[1][2] + v.z * m[2][2]
	};
}

inline D3DVECTOR Normalize(const D3DVECTOR& v)
{
	float len = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
	if (len > 0.0f) {
		float invLen = 1.0f / len;
		return {v.x * invLen, v.y * invLen, v.z * invLen};
	}
	return {0, 0, 0};
}

inline float DotProduct(const D3DVECTOR& a, const D3DVECTOR& b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline D3DVECTOR CrossProduct(const D3DVECTOR& a, const D3DVECTOR& b)
{
	return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

inline D3DVECTOR TransformPoint(const D3DVECTOR& p, const D3DRMMATRIX4D& m)
{
	return {
		p.x * m[0][0] + p.y * m[1][0] + p.z * m[2][0] + m[3][0],
		p.x * m[0][1] + p.y * m[1][1] + p.z * m[2][1] + m[3][1],
		p.x * m[0][2] + p.y * m[1][2] + p.z * m[2][2] + m[3][2]
	};
}

inline void MultiplyMatrix(D3DRMMATRIX4D out, const D3DRMMATRIX4D a, const D3DRMMATRIX4D b)
{
	D3DRMMATRIX4D res;
	for (int i = 0; i < 4; ++i) {
		float a0 = a[i][0], a1 = a[i][1], a2 = a[i][2], a3 = a[i][3];
		res[i][0] = a0 * b[0][0] + a1 * b[1][0] + a2 * b[2][0] + a3 * b[3][0];
		res[i][1] = a0 * b[0][1] + a1 * b[1][1] + a2 * b[2][1] + a3 * b[3][1];
		res[i][2] = a0 * b[0][2] + a1 * b[1][2] + a2 * b[2][2] + a3 * b[3][2];
		res[i][3] = a0 * b[0][3] + a1 * b[1][3] + a2 * b[2][3] + a3 * b[3][3];
	}
	memcpy(out, res, sizeof(D3DRMMATRIX4D));
}
