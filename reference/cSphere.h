#ifndef __CSPHERE_H__
#define __CSPHERE_H__

#include "cVec.h"
#include "cMat.h"
#include <assert.h>

class cAABB;
const float mRadiusEpsilon = 1e-2f;

class 
    cSphere
{
public:
    cVec3 mPos;
    float mRadius;

public:
    __forceinline cSphere() : mRadius(0.0f)
    {}

    __forceinline cSphere(const cVec3 &Pos, float Radius) : mPos(Pos), mRadius(Radius)
    {}

    __forceinline cSphere(const cSphere &Other) : mPos(Other.mPos), mRadius(Other.mRadius)
    {}

    __forceinline cSphere(const cVec3 &P0) : mPos(P0), mRadius(mRadiusEpsilon)
    {}

    __forceinline cSphere(const cVec3 &P0, const cVec3 &P1)
    {
        cVec3 a = P1 - P0;

        cVec3 o = a * 0.5f;

        mRadius = o.Length() + mRadiusEpsilon;
        mPos = P0 + o;
    }

    cSphere(const cVec3 &P0, const cVec3 &P1, const cVec3 &P2);
    cSphere(const cVec3 &O, const cVec3 &A, const cVec3 &B, const cVec3 &C);

public:
    __forceinline float SurfaceArea()
    {
        return 4.0f * Math::Pi * mRadius * mRadius;
    }
    __forceinline float Volume()
    {
        return 1.3333f * Math::Pi * mRadius * mRadius * mRadius;
    }
    __forceinline float CrossSectionalSurfaceArea()
    {
        return  Math::Pi * mRadius * mRadius;
    }

    __forceinline float Distance(cVec3 &Point)
    {
        return (mPos - Point).Length() - mRadius;
    }

    __forceinline bool Overlaps(const cSphere &Sphere) const
    {
        cVec3 v = Sphere.mPos - mPos;
        float d = mRadius + Sphere.mRadius;
        return v.LengthSqr() <= d*d;
    }

//     bool Intersects(const cRay &Ray) const
//     {
//         // geometric solution
//         cVec3 L = mPos - Ray.mOrigin;
//         float tca =  L * Ray.mNormalisedDirection;
//         if (tca < 0)
//         {
//             return L.LengthSqr() < mRadius * mRadius;
//         }
// 
//         float d2 = L * L - tca * tca;
//         if (d2 > mRadius * mRadius)
//         {
//             return false;
//         }
// 
//         return true;
//         float thc = sqrtf(mRadius*mRadius - d2);
//         float t0 = tca - thc;
//         float t1 = tca + thc;
// 
//         return true;
//     }


//     __forceinline float GetIntersection(const cRay &Ray) const
//     {
//         // geometric solution
//         cVec3 L = mPos - Ray.mOrigin;
//         float tca = L * Ray.mDirection;
//         if (tca < 0)
//         {
//             return -1.0f;
//         }
// 
//         float d2 = L * L - tca * tca;
//         if (d2 > mRadius * mRadius)
//         {
//             return -1.0f;
//         }
// 
//         float thc = sqrtf(mRadius*mRadius - d2);
//         float t0 = tca - thc;
//     //    float t1 = tca + thc;
// 
//         return t0;
//     }

    cAABB GetAABB();

    // Signed distance to point
    float Distance(const cVec3 &P) const
    {
        return (P - mPos).Length() - mRadius;
    }
    float Distance2(const cVec3 &P) const
    {
        return (P - mPos).LengthSqr() - mRadius * mRadius;
    }

    static cSphere RecurseMiniBall(cVec3 *P[], unsigned int p, unsigned int b = 0)
    {
        cSphere MB;

        switch (b)
        {
        case 0:
            MB = cSphere();
            break;
        case 1:
            MB = cSphere(*P[-1]);
            break;
        case 2:
            MB = cSphere(*P[-1], *P[-2]);
            break;
        case 3:
            MB = cSphere(*P[-1], *P[-2], *P[-3]);
            break;
        case 4:
            MB = cSphere(*P[-1], *P[-2], *P[-3], *P[-4]);
            return MB;
        }

        for (unsigned int i = 0; i < p; i++)
        {
            if (MB.Distance2(*P[i]) > 0.0f)   // Signed square distance to sphere
            {
                for (unsigned int j = i; j > 0; j--)
                {
                    cVec3 *T = P[j];
                    P[j] = P[j - 1];
                    P[j - 1] = T;
                }

                MB = RecurseMiniBall(P + 1, i, b + 1);
                assert(MB.mPos.x == MB.mPos.x);
            }
        }

        return MB;
    }

    static cSphere MiniBall(cVec3 P[], unsigned int p)
    {
        cVec3 **L = tracked_new cVec3*[p];

        for (unsigned int i = 0; i < p; i++)
            L[i] = &P[i];

        cSphere MB = RecurseMiniBall(L, p);

        delete[] L;

        return MB;
    }
};

#endif