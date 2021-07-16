#ifndef __CQUAT_H__
#define __CQUAT_H__

#include "Math.h"
#include "cVec.h"
#include "cMat.h"
#include "cEuler.h"

class cQuat
{
public:
    union
    {
        struct
        {
            cVec3 im;
            float re;
        };
        struct
        {
            cVec4 mQuat;
        };
    };

public:
    __forceinline cQuat( )                                                                    : mQuat(0.0f, 0.0f, 0.0f, 1.0f)    {}
    __forceinline cQuat( const cQuat &q )                                                     : mQuat(q.mQuat)                {}
    __forceinline cQuat( const float inW, const float inX, const float inY, const float inZ ) : mQuat(inX, inY, inZ, inW)        {}
    __forceinline cQuat( const float r, const cVec3 &i )                                      : mQuat(i.x, i.y, i.z, r)                    {}
    __forceinline cQuat( const cVec3 &i )                                                     : mQuat(i.x, i.y, i.z, 0.0f)                    {}
    __forceinline cQuat( const float r )                                                      : mQuat(0.0f, 0.0f, 0.0f, r)    {}

    __forceinline cQuat(const cVec3 &From, const cVec3 &To);        // Rotation quat from 'From' to 'To'
    __forceinline cQuat(const cVec3 &Position, const cVec3 &Up, const cVec3 &Target); // Lookat Quaternion
    __forceinline cQuat( const cEuler &e);                            // Convert from Euler
    __forceinline cQuat(const cVec3 &axis, const float theta);        // Convert from Angle/Axis

    __forceinline ~cQuat() {}

    __forceinline void         set( float inW, float inX, float inY, float inZ ) { mQuat.Set(inX, inY, inZ, inW); }
    __forceinline void         set( float inW, const cVec3 &inV                ) { re = inW; im = inV; }

    __forceinline void         identity() { re = 1.0f; im.Set(0.0f, 0.0f, 0.0f); }   // Set Identity Rotation (Multiplication)

    __forceinline float        length    () const { return sqrtf((re*re) + im.LengthSqr()); }
    __forceinline float        LengthSqr () const { return (re*re) + im.LengthSqr(); }
    __forceinline cQuat        &Normalise();
    __forceinline cQuat        Normalised() const;

    __forceinline void         rotate ( cVec3 &axis, float theta ) { (*this) = cQuat(axis, theta) * (*this); }
    __forceinline void         xRotate( float theta )              { (*this) = cQuat(cosf(theta * 0.5f), sinf(theta * 0.5f), 0.0f, 0.0f) * (*this); }
    __forceinline void         yRotate( float theta )              { (*this) = cQuat(cosf(theta * 0.5f), 0.0f, sinf(theta * 0.5f), 0.0f) * (*this); }
    __forceinline void         zRotate( float theta )              { (*this) = cQuat(cosf(theta * 0.5f), 0.0f, 0.0f, sinf(theta * 0.5f)) * (*this);    }

    __forceinline bool         IsZero    () const { return ((re == 0.0f) && im.IsZero()); }
    __forceinline bool         isIdentity() const { return ((re == 1.0f) && im.IsZero()); }
    __forceinline bool         isPure    () const { return (im.IsZero()); }
    __forceinline bool         isReal    () const { return (re == 0.0f);    }

    __forceinline void            toAxisAngle( cVec3 &axis, float &theta ) const;
    __forceinline void            toSpherical( float &latitude, float &longitude ) const;
    __forceinline void            toMat4     ( cMat4 &mat ) const;
    __forceinline void            toMat3     ( cMat3 &mat ) const;
    __forceinline void            FromMat3   ( const cMat3 &mat );
    inline void                   FromMat4   ( const cMat4 &mat );

    __forceinline void            ToEulerXYZ    (cEuler &e                                    );
    __forceinline void            FromEulerXYZ(cEuler e);
    __forceinline void            FromEulerXZY(cEuler e);
    __forceinline void            FromEulerYXZ(cEuler e);
    __forceinline void            FromEulerYZX(cEuler e);
    __forceinline void            FromEulerZXY(cEuler e);
    __forceinline void            FromEulerZYX(cEuler e);
    
    __forceinline void            FromEuler    (
        cEuler e
        );

    __forceinline cQuat &  operator = (const cQuat& v) { mQuat.x = v.mQuat.x; mQuat.y = v.mQuat.y; mQuat.z = v.mQuat.z; mQuat.w = v.mQuat.w; return *this; }

    __forceinline cQuat    operator *   (const cQuat& q) const;
    __forceinline cQuat    operator /   (const cQuat& q) const;
    __forceinline cQuat    operator +   (const cQuat& q) const;
    __forceinline cQuat    operator -   (const cQuat& q) const;

    __forceinline cQuat    &operator *= (const cQuat& q);
    __forceinline cQuat    &operator /= (const cQuat& q);
    __forceinline cQuat    &operator += (const cQuat& q);
    __forceinline cQuat    &operator -= (const cQuat& q);

    __forceinline cQuat    operator *   (const float r)    const;
    __forceinline cQuat    operator /   (const float r)    const;
    __forceinline cQuat    &operator *= (const float r);
    __forceinline cQuat    &operator /= (const float r);

    __forceinline cVec3    operator *   (const cVec3 &v) const;
    __forceinline cVec4    operator *   (const cVec4 &v) const;

    __forceinline cQuat    operator ~   () const { return cQuat(re, -im); }        // conjugate
    __forceinline cQuat    operator !   () const { const float len = 1.0f / LengthSqr(); return cQuat(-im.x * len, -im.y * len, -im.z * len, re * len); }        // inverse

    __forceinline bool     operator ==  (const cQuat& Other) { return mQuat == Other.mQuat; }
    __forceinline bool     operator !=  (const cQuat& Other) { return mQuat != Other.mQuat; }

};

#include "cQuat.inl"

#endif
