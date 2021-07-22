#include "cPlane.h"
#include <assert.h>

inline cQuat::cQuat(const cVec3 &From, const cVec3 &To)
{
	*this = cQuat(From ^ To, -acosf(From * To));
}

inline cQuat::cQuat(const cVec3 &Position, const cVec3 &UpVector, const cVec3 &TargetPosition)
{
	cVec3 forward(0.0f, 0.0f, -1.0f);  // world space forward of itentiy quat
	cPlane plane(UpVector, Position);

	cVec3 projectedTarget = plane.ProjectPointOntoPlane(TargetPosition);
	cVec3 projectedDirection = (projectedTarget - Position).Normalised();
	
	cQuat q1 = cQuat(forward, projectedDirection);
	cQuat q2 = cQuat(projectedDirection, (TargetPosition - Position).Normalised());

	*this = q1 * q2;
}


inline cQuat::cQuat(const cEuler &e)
{
	// FROM : http://www.cs.ualberta.ca/~andreas/math/matrfaq_latest.html
	const float hPitch	= e.pitch	* 0.5f;
	const float hYaw		= e.yaw		* 0.5f;
	const float hRoll	= e.roll	* 0.5f;

	const float fSinPitch		(sinf(hPitch));
	const float fCosPitch		(cosf(hPitch));
	const float fSinYaw			(sinf(hYaw));
	const float fCosYaw			(cosf(hYaw));
	const float fSinRoll			(sinf(hRoll));
	const float fCosRoll			(cosf(hRoll));
	const float fCosPitchCosYaw	(fCosPitch * fCosYaw);
	const float fSinPitchSinYaw	(fSinPitch * fSinYaw);

	re   = fCosRoll * fCosPitchCosYaw     + fSinRoll * fSinPitchSinYaw;
	im.x = fSinRoll * fCosPitchCosYaw     - fCosRoll * fSinPitchSinYaw;
	im.y = fCosRoll * fSinPitch * fCosYaw + fSinRoll * fCosPitch * fSinYaw;
	im.z = fCosRoll * fCosPitch * fSinYaw - fSinRoll * fSinPitch * fCosYaw;
}

inline cQuat::cQuat(const cVec3 &axis, const float theta)
{
	// FROM : http://www.cs.ualberta.ca/~andreas/math/matrfaq_latest.html

	const cVec3 tAxis(axis.Normalised());
	const float tSin = sinf(theta * 0.5f);

	re   = cosf(theta * 0.5f);
	im.x = tAxis.x * tSin;
	im.y = tAxis.y * tSin;
	im.z = tAxis.z * tSin;

	// Now normalize the Quaternion in case any values are very close to zero
	Normalise();
}

inline cQuat &cQuat::Normalise()
{
	float m = length();
	if (m > 0.0f) {
		m = 1.0f / m;
		re *= m;
		im *= m;
	} else {
		re = 0.0f;
		im.Set(0.0f, 0.0f, 0.0f);
	}

	return *this;
}

inline cQuat cQuat::Normalised() const
{
	const float m = length();
	if (m > 0.0f) {
		return *this / m;
	} else {
		return cQuat(0.0f, 0.0f, 0.0f, 0.0f);
	}
}

void cQuat::toAxisAngle(cVec3 &axis, float &theta) const
{
	theta = 2.0f * acosf(re);
	float s = sinf(theta * 0.5f);
	axis = im / s;
}

void cQuat::toSpherical(float &latitude, float &longitude ) const
{
	cVec3 axis;
	float angle;
	toAxisAngle(axis, angle);

	latitude = -asinf(axis.y);

	if (axis.x * axis.x + axis.z * axis.z < 0.0001f)
		longitude = 0.0f;
	else 
	{
		longitude = atan2f(axis.x, axis.z);
		if(longitude < 0.0) longitude += Math::Pi * 2.0f;
	}
}

void cQuat::toMat4 ( cMat4 &mat ) const
{
//	mat = cMat4::identMat();

/*	cQuat q;
	q.mQuat.x = im.x;
	q.mQuat.y = im.y;
	q.mQuat.z = im.z;
	q.mQuat.w = re;

	float sqw = q.mQuat.w*q.mQuat.w;
	float sqx = q.mQuat.x*q.mQuat.x;
	float sqy = q.mQuat.y*q.mQuat.y;
	float sqz = q.mQuat.z*q.mQuat.z;

	// invs (inverse square length) is only required if quaternion is not already Normalised
	float invs = 1 / (sqx + sqy + sqz + sqw);
	mat.m00 = ( sqx - sqy - sqz + sqw) * invs ; // since sqw + sqx + sqy + sqz =1/invs*invs
	mat.m11 = (-sqx + sqy - sqz + sqw) * invs ;
	mat.m22 = (-sqx - sqy + sqz + sqw) * invs ;
    
	float tmp1 = q.mQuat.x*q.mQuat.y;
	float tmp2 = q.mQuat.z*q.mQuat.w;
	mat.m10 = 2.0f * (tmp1 + tmp2)*invs ;
	mat.m01 = 2.0f * (tmp1 - tmp2)*invs ;
    
	tmp1 = q.mQuat.x*q.mQuat.z;
	tmp2 = q.mQuat.y*q.mQuat.w;
	mat.m20 = 2.0f * (tmp1 - tmp2)*invs ;
	mat.m02 = 2.0f * (tmp1 + tmp2)*invs ;

	tmp1 = q.mQuat.y*q.mQuat.z;
	tmp2 = q.mQuat.x*q.mQuat.w;
	mat.m21 = 2.0f * (tmp1 + tmp2)*invs ;
	mat.m12 = 2.0f * (tmp1 - tmp2)*invs ;*/

	// from bullet
	float d = mQuat.LengthSqr();
	if (d == 0.0f)
	{
		return;
	}

	float s = 2.0f / d;

	float xs = mQuat.x * s,  ys = mQuat.y * s,  zs = mQuat.z * s;
	float wx = mQuat.w * xs, wy = mQuat.w * ys, wz = mQuat.w * zs;
	float xx = mQuat.x * xs, xy = mQuat.x * ys, xz = mQuat.x * zs;
	float yy = mQuat.y * ys, yz = mQuat.y * zs, zz = mQuat.z * zs;
	mat.SetValue(
		1.0f - (yy + zz), xy - wz, xz + wy,
		xy + wz, 1.0f - (xx + zz), yz - wx,
		xz - wy, yz + wx, 1.0f - (xx + yy));
}

void cQuat::toMat3(cMat3 &mat) const
{
//	mat = cMat3::identMat();

// 	cQuat q;
// 	q.mQuat.x = im.x;
// 	q.mQuat.y = im.y;
// 	q.mQuat.z = im.z;
// 	q.mQuat.w = re;
// 
// 	float sqw = q.mQuat.w*q.mQuat.w;
// 	float sqx = q.mQuat.x*q.mQuat.x;
// 	float sqy = q.mQuat.y*q.mQuat.y;
// 	float sqz = q.mQuat.z*q.mQuat.z;
// 
// 	// invs (inverse square length) is only required if quaternion is not already Normalised
// 	float invs = 1 / (sqx + sqy + sqz + sqw);
// 	mat.m00 = (sqx - sqy - sqz + sqw) * invs; // since sqw + sqx + sqy + sqz =1/invs*invs
// 	mat.m11 = (-sqx + sqy - sqz + sqw) * invs;
// 	mat.m22 = (-sqx - sqy + sqz + sqw) * invs;
// 
// 	float tmp1 = q.mQuat.x*q.mQuat.y;
// 	float tmp2 = q.mQuat.z*q.mQuat.w;
// 	mat.m10 = 2.0f * (tmp1 + tmp2)*invs;
// 	mat.m01 = 2.0f * (tmp1 - tmp2)*invs;
// 
// 	tmp1 = q.mQuat.x*q.mQuat.z;
// 	tmp2 = q.mQuat.y*q.mQuat.w;
// 	mat.m20 = 2.0f * (tmp1 - tmp2)*invs;
// 	mat.m02 = 2.0f * (tmp1 + tmp2)*invs;
// 
// 	tmp1 = q.mQuat.y*q.mQuat.z;
// 	tmp2 = q.mQuat.x*q.mQuat.w;
// 	mat.m21 = 2.0f * (tmp1 + tmp2)*invs;
// 	mat.m12 = 2.0f * (tmp1 - tmp2)*invs;

// from bullet
	float d = mQuat.LengthSqr();
	assert(d != 0.0f);
	float s = 2.0f / d;

	float xs = mQuat.x * s, ys = mQuat.y * s, zs = mQuat.z * s;
	float wx = mQuat.w * xs, wy = mQuat.w * ys, wz = mQuat.w * zs;
	float xx = mQuat.x * xs, xy = mQuat.x * ys, xz = mQuat.x * zs;
	float yy = mQuat.y * ys, yz = mQuat.y * zs, zz = mQuat.z * zs;
	mat.SetValue(
		1.0f - (yy + zz), xy - wz, xz + wy,
		xy + wz, 1.0f - (xx + zz), yz - wx,
		xz - wy, yz + wx, 1.0f - (xx + yy));
}


inline void cQuat::FromMat3(const cMat3 &mat)
{
	// 	float trace = mat.m00 + mat.m11 + mat.m22;
	// 	if( trace > 0 )
	// 	{
	// 		float s = 0.5f / sqrtf(trace + 1.0f);
	// 		mQuat.x = 0.25f / s;
	// 		mQuat.y = ( mat.m21 - mat.m12 ) * s;
	// 		mQuat.z = ( mat.m02 - mat.m20 ) * s;
	// 		mQuat.w = ( mat.m10 - mat.m01 ) * s;
	// 	}
	// 	else
	// 	{
	// 		if ( mat.m00 > mat.m11 && mat.m00 > mat.m22 )
	// 		{
	// 			float s = 2.0f * sqrtf( 1.0f + mat.m00 - mat.m11 - mat.m22);
	// 			mQuat.x = (mat.m21 - mat.m12 ) / s;
	// 			mQuat.y = 0.25f * s;
	// 			mQuat.z = (mat.m01 + mat.m10 ) / s;
	// 			mQuat.w = (mat.m02 + mat.m20 ) / s;
	// 		}
	// 		else if (mat.m11 > mat.m22)
	// 		{
	// 			float s = 2.0f * sqrtf( 1.0f + mat.m11 - mat.m00 - mat.m22);
	// 			mQuat.x = (mat.m02 - mat.m20 ) / s;
	// 			mQuat.y = (mat.m01 + mat.m10 ) / s;
	// 			mQuat.z = 0.25f * s;
	// 			mQuat.w = (mat.m12 + mat.m21 ) / s;
	// 		}
	// 		else
	// 		{
	// 			float s = 2.0f * sqrtf( 1.0f + mat.m22 - mat.m00 - mat.m11 );
	// 			mQuat.x = (mat.m10 - mat.m01 ) / s;
	// 			mQuat.y = (mat.m02 + mat.m20 ) / s;
	// 			mQuat.z = (mat.m12 + mat.m21 ) / s;
	// 			mQuat.w = 0.25f * s;
	// 		}
	// 	}

		// from bullet3d
	float trace = mat.mRow0.x + mat.mRow1.y + mat.mRow2.z;

	float temp[4];

	if (trace > float(0.0))
	{
		float s = sqrtf(trace + float(1.0));
		temp[3] = (s * float(0.5));
		s = float(0.5) / s;

		temp[0] = ((mat.mRow2.y - mat.mRow1.z) * s);
		temp[1] = ((mat.mRow0.z - mat.mRow2.x) * s);
		temp[2] = ((mat.mRow1.x - mat.mRow0.y) * s);
	}
	else
	{
		int i = mat.mRow0.x < mat.mRow1.y ?
			(mat.mRow1.y < mat.mRow2.z ? 2 : 1) :
			(mat.mRow0.x < mat.mRow2.z ? 2 : 0);
		int j = (i + 1) % 3;
		int k = (i + 2) % 3;

		float s = sqrtf(mat.m33[i][i] - mat.m33[j][j] - mat.m33[k][k] + float(1.0));
		temp[i] = s * float(0.5);
		s = float(0.5) / s;

		temp[3] = (mat.m33[k][j] - mat.m33[j][k]) * s;
		temp[j] = (mat.m33[j][i] + mat.m33[i][j]) * s;
		temp[k] = (mat.m33[k][i] + mat.m33[i][k]) * s;
	}
	set(temp[3], temp[0], temp[1], temp[2]);
}

void cQuat::FromMat4 ( const cMat4 &mat )
{
// 	float trace = mat.m00 + mat.m11 + mat.m22;
// 	if( trace > 0 )
// 	{
// 		float s = 0.5f / sqrtf(trace + 1.0f);
// 		mQuat.x = 0.25f / s;
// 		mQuat.y = ( mat.m21 - mat.m12 ) * s;
// 		mQuat.z = ( mat.m02 - mat.m20 ) * s;
// 		mQuat.w = ( mat.m10 - mat.m01 ) * s;
// 	}
// 	else
// 	{
// 		if ( mat.m00 > mat.m11 && mat.m00 > mat.m22 )
// 		{
// 			float s = 2.0f * sqrtf( 1.0f + mat.m00 - mat.m11 - mat.m22);
// 			mQuat.x = (mat.m21 - mat.m12 ) / s;
// 			mQuat.y = 0.25f * s;
// 			mQuat.z = (mat.m01 + mat.m10 ) / s;
// 			mQuat.w = (mat.m02 + mat.m20 ) / s;
// 		}
// 		else if (mat.m11 > mat.m22)
// 		{
// 			float s = 2.0f * sqrtf( 1.0f + mat.m11 - mat.m00 - mat.m22);
// 			mQuat.x = (mat.m02 - mat.m20 ) / s;
// 			mQuat.y = (mat.m01 + mat.m10 ) / s;
// 			mQuat.z = 0.25f * s;
// 			mQuat.w = (mat.m12 + mat.m21 ) / s;
// 		}
// 		else
// 		{
// 			float s = 2.0f * sqrtf( 1.0f + mat.m22 - mat.m00 - mat.m11 );
// 			mQuat.x = (mat.m10 - mat.m01 ) / s;
// 			mQuat.y = (mat.m02 + mat.m20 ) / s;
// 			mQuat.z = (mat.m12 + mat.m21 ) / s;
// 			mQuat.w = 0.25f * s;
// 		}
// 	}

	// from bullet3d
/*	float trace = mat.mRow0.x + mat.mRow1.y + mat.mRow2.z;

	float temp[4];

	if (trace > float(0.0))
	{
		float s = sqrtf(trace + float(1.0));
		temp[3] = (s * float(0.5));
		s = float(0.5) / s;

		temp[0] = ((mat.mRow2.y - mat.mRow1.z) * s);
		temp[1] = ((mat.mRow0.z - mat.mRow2.x) * s);
		temp[2] = ((mat.mRow1.x - mat.mRow0.y) * s);
	}
	else
	{
		int i = mat.mRow0.x < mat.mRow1.y ?
			(mat.mRow1.y < mat.mRow2.z ? 2 : 1) :
			(mat.mRow0.x < mat.mRow2.z ? 2 : 0);
		int j = (i + 1) % 3;
		int k = (i + 2) % 3;

		float s = sqrtf(mat.m44[i][i] - mat.m44[j][j] - mat.m44[k][k] + float(1.0));
		temp[i] = s * float(0.5);
		s = float(0.5) / s;

		temp[3] = (mat.m44[k][j] - mat.m44[j][k]) * s;
		temp[j] = (mat.m44[j][i] + mat.m44[i][j]) * s;
		temp[k] = (mat.m44[k][i] + mat.m44[i][k]) * s;
	}
	set(temp[3], temp[0], temp[1], temp[2]);*/

	float t = 0.0f;
	if (mat.mRow2.z < 0) 
	{ 
		if (mat.mRow0.x > mat.mRow1.y)
		{
			t = 1.0f + mat.mRow0.x - mat.mRow1.y - mat.mRow2.z;
			mQuat.Set(t, mat.mRow1.x + mat.mRow0.y, mat.mRow0.z + mat.mRow2.x, mat.mRow2.y - mat.mRow1.z);
		} 
		else
		{ 
			t = 1.0f - mat.mRow0.x + mat.mRow1.y - mat.mRow2.z;
			mQuat.Set(mat.mRow1.x + mat.mRow0.y, t, mat.mRow2.y + mat.mRow1.z, mat.mRow0.z - mat.mRow2.x);
		} 
	}
	else
	{ 
		if (mat.mRow0.x < -mat.mRow1.y)
		{
			t = 1.0f - mat.mRow0.x - mat.mRow1.y + mat.mRow2.z;
			mQuat.Set(mat.mRow0.z + mat.mRow2.x, mat.mRow2.y + mat.mRow1.z, t, mat.mRow1.x - mat.mRow0.y);
		} 
		else 
		{ 
			t = 1.0f + mat.mRow0.x + mat.mRow1.y + mat.mRow2.z;
			mQuat.Set(mat.mRow2.y - mat.mRow1.z, mat.mRow0.z - mat.mRow2.x, mat.mRow1.x - mat.mRow0.y, t);
		} 
	}

	mQuat *= 0.5f / sqrtf(t);
}

inline void cQuat::ToEulerXYZ(cEuler &e)
{

	e.x = atan2f(-2.0f * (im.y*im.z - re*im.x), re*re - im.x*im.x - im.y*im.y + im.z*im.z);
	e.y = asinf(2.0f * (im.x*im.z + re*im.y));
	e.z = atan2f(-2.0f * (im.x*im.y - re*im.z), re*re + im.x*im.x - im.y*im.y - im.z*im.z);
}

inline void cQuat::FromEulerXYZ(cEuler e)
{
	const float hPitch = e.pitch	* 0.5f;
	const float hYaw = e.yaw		* 0.5f;
	const float hRoll = e.roll		* 0.5f;

	float c1 = (float)cosf(hPitch);
	float s1 = (float)sinf(hPitch);
	float c2 = (float)cosf(hYaw);
	float s2 = (float)sinf(hYaw);
	float c3 = (float)cosf(hRoll);
	float s3 = (float)sinf(hRoll);
	float c1c2 = c1 * c2;
	float s1s2 = s1 * s2;

	re = c1c2 * c3 - s1s2 * s3;
	im.x = s1 * c2 * c3 + c1 * s2 * s3;
	im.y = c1 * s2 * c3 - s1 * c2 * s3;
	im.z = c1c2 * s3 + s1s2 * c3;
}

inline void cQuat::FromEulerXZY(cEuler e)
{
	const float hPitch = e.pitch	* 0.5f;
	const float hYaw = e.yaw		* 0.5f;
	const float hRoll = e.roll		* 0.5f;

	float c1 = (float)cosf(hPitch);
	float s1 = (float)sinf(hPitch);
	float c2 = (float)cosf(hYaw);
	float s2 = (float)sinf(hYaw);
	float c3 = (float)cosf(hRoll);
	float s3 = (float)sinf(hRoll);
	float c1c2 = c1 * c2;
	float s1s2 = s1 * s2;

	re	 = c1c2 * c3 + s1s2 * s3;
	im.x = s1 * c2 * c3 - c1 * s2 * s3;
	im.y = c1c2 * s3 - s1s2 * c3;
	im.z = s1 * c2 * s3 + c1 * s2 * c3;
}


inline void cQuat::FromEulerYXZ(cEuler e)
{
	const float hPitch = e.pitch	* 0.5f;
	const float hYaw = e.yaw		* 0.5f;
	const float hRoll = e.roll		* 0.5f;

	float c1 = (float)cosf(hPitch);
	float s1 = (float)sinf(hPitch);
	float c2 = (float)cosf(hYaw);
	float s2 = (float)sinf(hYaw);
	float c3 = (float)cosf(hRoll);
	float s3 = (float)sinf(hRoll);
	float c1c2 = c1 * c2;
	float s1s2 = s1 * s2;

	re	 = c1c2 * c3 + s1s2 * s3;
	im.x = s1 * c2 * s3 + c1 * s2 * c3;
	im.y = s1 * c2 * c3 - c1 * s2 * s3;
	im.z = c1c2 * s3 - s1s2 * c3;
}


inline void cQuat::FromEulerYZX(cEuler e)
{
	const float hPitch = e.pitch	* 0.5f;
	const float hYaw = e.yaw		* 0.5f;
	const float hRoll = e.roll		* 0.5f;

	float c1 = (float)cosf(hPitch);
	float s1 = (float)sinf(hPitch);
	float c2 = (float)cosf(hYaw);
	float s2 = (float)sinf(hYaw);
	float c3 = (float)cosf(hRoll);
	float s3 = (float)sinf(hRoll);
	float c1c2 = c1 * c2;
	float s1s2 = s1 * s2;

	re	 = c1c2 * c3 - s1s2 * s3;
	im.x = s1s2 * c3 + c1c2 * s3;
	im.y = s1 * c2 * c3 + c1 * s2 * s3;
	im.z = c1 * s2 * c3 - s1 * c2 * s3;
}

inline void cQuat::FromEulerZXY(cEuler e)
{
	const float hPitch = e.pitch	* 0.5f;
	const float hYaw = e.yaw		* 0.5f;
	const float hRoll = e.roll		* 0.5f;

	float c1 = (float)cosf(hPitch);
	float s1 = (float)sinf(hPitch);
	float c2 = (float)cosf(hYaw);
	float s2 = (float)sinf(hYaw);
	float c3 = (float)cosf(hRoll);
	float s3 = (float)sinf(hRoll);
	float c1c2 = c1 * c2;
	float s1s2 = s1 * s2;

	re	 = c1c2 * c3 - s1s2 * s3;
	im.x = c1 * s2 * c3 - s1 * c2 * s3;
	im.y = s1s2 * c3 + c1c2 * s3;
	im.z = s1 * c2 * c3 - c1 * s2 * s3;
}

inline void cQuat::FromEulerZYX(cEuler e)
{
	const float hPitch = e.pitch	* 0.5f;
	const float hYaw = e.yaw		* 0.5f;
	const float hRoll = e.roll		* 0.5f;

	float c1 = (float)cosf(hPitch);
	float s1 = (float)sinf(hPitch);
	float c2 = (float)cosf(hYaw);
	float s2 = (float)sinf(hYaw);
	float c3 = (float)cosf(hRoll);
	float s3 = (float)sinf(hRoll);
	float c1c2 = c1 * c2;
	float s1s2 = s1 * s2;

	re	 = c1c2 * c3 + s1s2 * s3;
	im.x = c1c2 * s3 - s1s2 * c3;
	im.y = s1 * c2 * s3 + c1 * s2 * c3;
	im.z = s1 * c2 * c3 - c1 * s2 * s3;
}

inline void cQuat::FromEuler(cEuler e)
{
	// FROM : http://www.cs.ualberta.ca/~andreas/math/matrfaq_latest.html
	const float hPitch = e.pitch	* 0.5f;
	const float hYaw = e.yaw		* 0.5f;
	const float hRoll = e.roll	* 0.5f;

	const float fSinPitch(sinf(hPitch));
	const float fCosPitch(cosf(hPitch));
	const float fSinYaw(sinf(hYaw));
	const float fCosYaw(cosf(hYaw));
	const float fSinRoll(sinf(hRoll));
	const float fCosRoll(cosf(hRoll));
	const float fCosPitchCosYaw(fCosPitch * fCosYaw);
	const float fSinPitchSinYaw(fSinPitch * fSinYaw);

	re	 = fCosRoll * fCosPitchCosYaw + fSinRoll * fSinPitchSinYaw;
	im.x = fSinRoll * fCosPitchCosYaw - fCosRoll * fSinPitchSinYaw;
	im.y = fCosRoll * fSinPitch * fCosYaw + fSinRoll * fCosPitch * fSinYaw;
	im.z = fCosRoll * fCosPitch * fSinYaw - fSinRoll * fSinPitch * fCosYaw;
}

inline cQuat cQuat::operator * (const cQuat &q) const
{
	// 9 muls, 27 adds
	const float tmp_00 = (im.z - im.y) * (q.im.y - q.im.z);
	const float tmp_01 = (re   + im.x) * (q.re   + q.im.x);
	const float tmp_02 = (re   - im.x) * (q.im.y + q.im.z);
	const float tmp_03 = (im.y + im.z) * (q.re   - q.im.x);
	const float tmp_04 = (im.z - im.x) * (q.im.x - q.im.y);
	const float tmp_05 = (im.z + im.x) * (q.im.x + q.im.y);
	const float tmp_06 = (re   + im.y) * (q.re   - q.im.z);
	const float tmp_07 = (re   - im.y) * (q.re   + q.im.z);
	const float tmp_08 = tmp_05 + tmp_06 + tmp_07;
	const float tmp_09 = (tmp_04 + tmp_08) * 0.5f;

	return cQuat(
		tmp_00 + tmp_09 - tmp_05,
		tmp_01 + tmp_09 - tmp_08,
		tmp_02 + tmp_09 - tmp_07,
		tmp_03 + tmp_09 - tmp_06
		);
}

inline cQuat cQuat::operator / (const cQuat &q) const
{
	// Multiply p by the conjugate of q divided by the square of it's norm
	return (*this) * (!q / q.LengthSqr());
}

inline cQuat cQuat::operator + (const cQuat &q) const
{
	return cQuat(re + q.re, im + q.im);
}

inline cQuat cQuat::operator - (const cQuat &q) const
{
	return cQuat(re - q.re, im - q.im);
}

inline cQuat	&cQuat::operator *= (const cQuat &q)
{
	// 9 muls, 27 adds
	const float tmp_00 = (im.z - im.y) * (q.im.y - q.im.z);
	const float tmp_01 = (re   + im.x) * (q.re   + q.im.x);
	const float tmp_02 = (re   - im.x) * (q.im.y + q.im.z);
	const float tmp_03 = (im.y + im.z) * (q.re   - q.im.x);
	const float tmp_04 = (im.z - im.x) * (q.im.x - q.im.y);
	const float tmp_05 = (im.z + im.x) * (q.im.x + q.im.y);
	const float tmp_06 = (re   + im.y) * (q.re   - q.im.z);
	const float tmp_07 = (re   - im.y) * (q.re   + q.im.z);
	const float tmp_08 = tmp_05 + tmp_06 + tmp_07;
	const float tmp_09 = (tmp_04 + tmp_08) * 0.5f;

	set(tmp_00 + tmp_09 - tmp_05,
		tmp_01 + tmp_09 - tmp_08,
		tmp_02 + tmp_09 - tmp_07,
		tmp_03 + tmp_09 - tmp_06);

	return *this;
}

inline cQuat &cQuat::operator /= (const cQuat &q)
{
	// Multiply p by the conjugate of q divided by the square of it's norm
	(*this) *= (!q / q.LengthSqr());
	return *this;
}

inline cQuat &cQuat::operator += (const cQuat &q)
{
	re += q.re;
	im += q.im;
	return *this;
}

inline cQuat &cQuat::operator -= (const cQuat &q)
{
	re -= q.re;
	im -= q.im;
	return *this;
}

inline cQuat cQuat::operator * (const float r) const
{
	return cQuat(re * r, im * r);
}

inline cQuat cQuat::operator / (const float r) const
{
	const float t = 1.0f / r;
	return cQuat(re * t, im * t);
}

inline cQuat &cQuat::operator *= (const float r)
{
	re *= r;
	im *= r;
	return *this;
}

inline cQuat &cQuat::operator /= (const float r)
{
	const float t = 1.0f / r;
	re *= t;
	im *= t;
	return *this;
}

inline cVec3 cQuat::operator * (const cVec3 &v) const
{
	cQuat q(0.0, v.x, v.y, v.z);
	cQuat res = ((~*this) * q * *this);
	return res.im;
}

inline cVec4 cQuat::operator * (const cVec4 &v) const
{
	cQuat q(0.0, v.x, v.y, v.z);
	cVec3 ret = ((~*this) * q * *this).im;
	return cVec4 ( ret.x, ret.y, ret.z, v.w );
}
