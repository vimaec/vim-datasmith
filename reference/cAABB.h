#ifndef __CAABB_H__
#define __CAABB_H__

#include "cVec.h"
//#include "cRay.h"
#include "cSphere.h"

#include <float.h>

class cAABB;


template < class type >
class cAABBImpl
{
public:
    __forceinline cAABBImpl ( type Min, type Max ) : mMin ( Min ), mMax ( Max ) { }
    __forceinline cAABBImpl() {}
    __forceinline ~cAABBImpl ( ) {}

    __forceinline cAABBImpl& operator = (const cAABBImpl & Other)
    {
        mMin = Other.mMin;
        mMax = Other.mMax;

        return *this;
    }

    __forceinline bool IsValid ( )
    {
        return mMin.x <= mMax.x && mMin.y <= mMax.y && mMin.z <= mMax.z;
    }

    union
    {
        struct
        {
            type mBounds [ 2 ];
        };
        struct
        {
            type mMin;
            type mMax;
        };
    };

    __forceinline type GetCenter ( ) const
    {
        return ( mMin + mMax ) / 2;
    }

    __forceinline type GetSize ( ) const
    {
        return ( -mMin + mMax );
    }

    __forceinline void Add(const cAABBImpl &Other)
    {
        mMin.x = Utils::Min(mMin.x, Other.mMin.x);
        mMin.y = Utils::Min(mMin.y, Other.mMin.y);
        mMin.z = Utils::Min(mMin.z, Other.mMin.z);
        mMax.x = Utils::Max(mMax.x, Other.mMax.x);
        mMax.y = Utils::Max(mMax.y, Other.mMax.y);
        mMax.z = Utils::Max(mMax.z, Other.mMax.z);
    }

    __forceinline void Add(const type &Obj)
    {
        mMin.x = Utils::Min(mMin.x, Obj.x);
        mMin.y = Utils::Min(mMin.y, Obj.y);
        mMin.z = Utils::Min(mMin.z, Obj.z);
        mMax.x = Utils::Max(mMax.x, Obj.x);
        mMax.y = Utils::Max(mMax.y, Obj.y);
        mMax.z = Utils::Max(mMax.z, Obj.z);
    }

    __forceinline bool Overlaps(const cAABBImpl &Other) const
    {

        return
            (mMin.x <= Other.mMax.x) && (mMax.x >= Other.mMin.x) &&
            (mMin.y <= Other.mMax.y) && (mMax.y >= Other.mMin.y) &&
            (mMin.z <= Other.mMax.z) && (mMax.z >= Other.mMin.z);
    }

    __forceinline bool Contains ( const cAABBImpl &Other ) const
    {
        return 
            (mMin.x <= Other.mMin.x) && (mMax.x >= Other.mMax.x) &&
            (mMin.y <= Other.mMin.y) && (mMax.y >= Other.mMax.y) &&
            (mMin.z <= Other.mMin.z) && (mMax.z >= Other.mMax.z);
    }

    static inline float squared(float v) { return v * v; }

    __forceinline bool Contains(const cSphere &Sphere) const
    {
        float dist_squared = Sphere.mRadius * Sphere.mRadius;

        if (Sphere.mPos.x < mMin.x) dist_squared -= squared(Sphere.mPos.x - mMin.x);
        else if (Sphere.mPos.x > mMax.x) dist_squared -= squared(Sphere.mPos.x - mMax.x);
        if (Sphere.mPos.y < mMin.y) dist_squared -= squared(Sphere.mPos.y - mMin.y);
        else if (Sphere.mPos.y > mMax.y) dist_squared -= squared(Sphere.mPos.y - mMax.y);
        if (Sphere.mPos.z < mMin.z) dist_squared -= squared(Sphere.mPos.z - mMin.z);
        else if (Sphere.mPos.z > mMax.z) dist_squared -= squared(Sphere.mPos.z - mMax.z);
        return dist_squared > 0;
    }

    static void ProjectTriToAxis(const cVec3 &axis, float &dMin, float &dMax, const cVec3 &v0, const cVec3 &v1, const cVec3 &v2)
    {
        dMin = dMax = axis* v0;
        float t = axis * v1;
        dMin = Utils::Min(t, dMin);
        dMax = Utils::Max(t, dMax);
        t = axis * v2;
        dMin = Utils::Min(t, dMin);
        dMax = Utils::Max(t, dMax);
    }

    
    void ProjectToAxis(const cVec3 &axis, float &dMin, float &dMax) const
    {
        cVec3 c = GetCenter();
        cVec3 e = GetSize() * 0.5f;

        // Compute the projection interval radius of the AABB onto L(t) = aabb.center + t * plane.normal;
        float r = e[0] * fabsf(axis[0]) + e[1] * fabsf(axis[1]) + e[2] * fabsf(axis[2]);
        // Compute the distance of the box center from plane.
        float s = axis* c;
        dMin = s - r;
        dMax = s + r;
        if (dMin > dMax)
        {
            float t = dMin;
            dMin = dMax;
            dMax = t;
        }
    }

    static bool RangesOverlap(float r0, float r1, float t0, float t1)
    {
        return (r0 <= t1) && (r1 >= t0);
    }

    __forceinline bool Contains(const cVec3 &v0, const cVec3 &v1, const cVec3 &v2 ) const
    {
//         cVec3 points[8];
//         points[0].Set(mMin.x, mMin.y, mMin.z);
//         points[1].Set(mMin.x, mMin.y, mMax.z);
//         points[2].Set(mMin.x, mMax.y, mMin.z);
//         points[3].Set(mMin.x, mMax.y, mMax.z);
//         points[4].Set(mMax.x, mMin.y, mMin.z);
//         points[5].Set(mMax.x, mMin.y, mMax.z);
//         points[6].Set(mMax.x, mMax.y, mMin.z);
//         points[7].Set(mMax.x, mMax.y, mMax.z);
// 
//         cVec3 projN = (v0 - v1) ^ (v0 - v2);
//         float triProj = v0 * projN;
//         float triProj1 = v1 * projN;
//         float triProj2 = v2 * projN;
// 
//         float side = 0;
//         side += (points[0] * projN) > triProj ? 1 : -1;
//         side += (points[1] * projN) > triProj ? 1 : -1;
//         side += (points[2] * projN) > triProj ? 1 : -1;
//         side += (points[3] * projN) > triProj ? 1 : -1;
//         side += (points[4] * projN) > triProj ? 1 : -1;
//         side += (points[5] * projN) > triProj ? 1 : -1;
//         side += (points[6] * projN) > triProj ? 1 : -1;
//         side += (points[7] * projN) > triProj ? 1 : -1;
// 
//         return side != 8 && side != -8;
              float t1, t2, a1, a2;
        const cVec3 e[3] = { cVec3(1,0,0), cVec3(0,1,0), cVec3(0,0,1) };

        for (int i = 0; i < 3; ++i)
        {
            ProjectTriToAxis(e[i], t1, t2, v0, v1, v2);
            ProjectToAxis(e[i], a1, a2);
            if (!RangesOverlap(t1, t2, a1, a2))
                return false;
        }

        cVec3 n = (v1 - v0) ^ (v2 - v0);
        ProjectTriToAxis(n, t1, t2, v0, v1, v2);
        ProjectToAxis(n, a1, a2);
        if (!RangesOverlap(t1, t2, a1, a2))
            return false;

        const cVec3 t[3] = { v1-v0, v2-v0, v2-v1 };

        for (int i = 0; i < 3; ++i)
        {

            for (int j = 0; j < 3; ++j)
            {
                cVec3 axis = e[i] ^ t[j];
                float len = axis.LengthSqr();
                if (len <= 1e-4f)
                    continue; // Ignore tests on degenerate axes.

                ProjectTriToAxis(axis, t1, t2, v0, v1, v2);
                ProjectToAxis(axis, a1, a2);
                if (!RangesOverlap(t1, t2, a1, a2))
                    return false;
            }
        }

        // No separating axis exists, the AABB and triangle intersect.
        return true;
    }

    __forceinline bool Contains ( const type &Point ) const
    {
        return
            (mMin.x <= Point.x) && (mMax.x >= Point.x) &&
            (mMin.y <= Point.y) && (mMax.y >= Point.y) &&
            (mMin.z <= Point.z) && (mMax.z >= Point.z);
    }
};

class
    cAABB
{
public:
    __forceinline cAABB(cVec3 Min, cVec3 Max) : mMin(Min), mMax(Max) { }
    __forceinline cAABB(const cAABB& Other) : mMin(Other.mMin), mMax(Other.mMax) { }
    __forceinline cAABB() {}
    __forceinline ~cAABB() {}

    __forceinline void operator = (const cAABB& Other) { mMin = Other.mMin; mMax = Other.mMax; }

    __forceinline bool IsValid()
    {
        return mMin.x <= mMax.x && mMin.y <= mMax.y && mMin.z <= mMax.z;
    }

    union
    {
        struct
        {
            cVec3 mBounds[2];
        };
        struct
        {
            cVec3 mMin;
            cVec3 mMax;
        };
    };

    bool Intersects(const cVec3 & v0, const cVec3 & v1, const cVec3 & v2) const;
//    bool Intersects(const cRay & Ray) const;

    __forceinline cVec3 GetCenter() const
    {
        return (mMin + mMax) / 2;
    }

    __forceinline cVec3 GetSize() const
    {
        return (-mMin + mMax);
    }

    __forceinline void Add(const cAABB &Other)
    {
        mMin.x = Utils::Min(mMin.x, Other.mMin.x);
        mMin.y = Utils::Min(mMin.y, Other.mMin.y);
        mMin.z = Utils::Min(mMin.z, Other.mMin.z);
        mMax.x = Utils::Max(mMax.x, Other.mMax.x);
        mMax.y = Utils::Max(mMax.y, Other.mMax.y);
        mMax.z = Utils::Max(mMax.z, Other.mMax.z);
    }

    __forceinline void Add(const cVec3 &Obj)
    {
        mMin.x = Utils::Min(mMin.x, Obj.x);
        mMin.y = Utils::Min(mMin.y, Obj.y);
        mMin.z = Utils::Min(mMin.z, Obj.z);
        mMax.x = Utils::Max(mMax.x, Obj.x);
        mMax.y = Utils::Max(mMax.y, Obj.y);
        mMax.z = Utils::Max(mMax.z, Obj.z);
    }

    __forceinline bool Overlaps(const cAABB &Other) const
    {

        return
            (mMin.x <= Other.mMax.x) && (mMax.x >= Other.mMin.x) &&
            (mMin.y <= Other.mMax.y) && (mMax.y >= Other.mMin.y) &&
            (mMin.z <= Other.mMax.z) && (mMax.z >= Other.mMin.z);
    }

    __forceinline bool Contains(const cAABB &Other) const
    {
        return
            (mMin.x <= Other.mMin.x) && (mMax.x >= Other.mMax.x) &&
            (mMin.y <= Other.mMin.y) && (mMax.y >= Other.mMax.y) &&
            (mMin.z <= Other.mMin.z) && (mMax.z >= Other.mMax.z);
    }

    static inline float squared(float v) { return v * v; }

    __forceinline bool Contains(const cSphere &Sphere) const
    {
        float dist_squared = Sphere.mRadius * Sphere.mRadius;

        if (Sphere.mPos.x < mMin.x) dist_squared -= squared(Sphere.mPos.x - mMin.x);
        else if (Sphere.mPos.x > mMax.x) dist_squared -= squared(Sphere.mPos.x - mMax.x);
        if (Sphere.mPos.y < mMin.y) dist_squared -= squared(Sphere.mPos.y - mMin.y);
        else if (Sphere.mPos.y > mMax.y) dist_squared -= squared(Sphere.mPos.y - mMax.y);
        if (Sphere.mPos.z < mMin.z) dist_squared -= squared(Sphere.mPos.z - mMin.z);
        else if (Sphere.mPos.z > mMax.z) dist_squared -= squared(Sphere.mPos.z - mMax.z);
        return dist_squared > 0;
    }

    static void ProjectTriToAxis(const cVec3 &axis, float &dMin, float &dMax, const cVec3 &v0, const cVec3 &v1, const cVec3 &v2)
    {
        dMin = dMax = axis* v0;
        float t = axis * v1;
        dMin = Utils::Min(t, dMin);
        dMax = Utils::Max(t, dMax);
        t = axis * v2;
        dMin = Utils::Min(t, dMin);
        dMax = Utils::Max(t, dMax);
    }


    void ProjectToAxis(const cVec3 &axis, float &dMin, float &dMax) const
    {
        cVec3 c = GetCenter();
        cVec3 e = GetSize() * 0.5f;

        // Compute the projection interval radius of the AABB onto L(t) = aabb.center + t * plane.normal;
        float r = e[0] * fabsf(axis[0]) + e[1] * fabsf(axis[1]) + e[2] * fabsf(axis[2]);
        // Compute the distance of the box center from plane.
        float s = axis* c;
        dMin = s - r;
        dMax = s + r;
        if (dMin > dMax)
        {
            float t = dMin;
            dMin = dMax;
            dMax = t;
        }
    }

    static bool RangesOverlap(float r0, float r1, float t0, float t1)
    {
        return (r0 <= t1) && (r1 >= t0);
    }

    __forceinline bool Contains(const cVec3 &v0, const cVec3 &v1, const cVec3 &v2) const
    {
        //         cVec3 points[8];
        //         points[0].Set(mMin.x, mMin.y, mMin.z);
        //         points[1].Set(mMin.x, mMin.y, mMax.z);
        //         points[2].Set(mMin.x, mMax.y, mMin.z);
        //         points[3].Set(mMin.x, mMax.y, mMax.z);
        //         points[4].Set(mMax.x, mMin.y, mMin.z);
        //         points[5].Set(mMax.x, mMin.y, mMax.z);
        //         points[6].Set(mMax.x, mMax.y, mMin.z);
        //         points[7].Set(mMax.x, mMax.y, mMax.z);
        // 
        //         cVec3 projN = (v0 - v1) ^ (v0 - v2);
        //         float triProj = v0 * projN;
        //         float triProj1 = v1 * projN;
        //         float triProj2 = v2 * projN;
        // 
        //         float side = 0;
        //         side += (points[0] * projN) > triProj ? 1 : -1;
        //         side += (points[1] * projN) > triProj ? 1 : -1;
        //         side += (points[2] * projN) > triProj ? 1 : -1;
        //         side += (points[3] * projN) > triProj ? 1 : -1;
        //         side += (points[4] * projN) > triProj ? 1 : -1;
        //         side += (points[5] * projN) > triProj ? 1 : -1;
        //         side += (points[6] * projN) > triProj ? 1 : -1;
        //         side += (points[7] * projN) > triProj ? 1 : -1;
        // 
        //         return side != 8 && side != -8;
        float t1, t2, a1, a2;
        const cVec3 e[3] = { cVec3(1,0,0), cVec3(0,1,0), cVec3(0,0,1) };

        for (int i = 0; i < 3; ++i)
        {
            ProjectTriToAxis(e[i], t1, t2, v0, v1, v2);
            ProjectToAxis(e[i], a1, a2);
            if (!RangesOverlap(t1, t2, a1, a2))
                return false;
        }

        cVec3 n = (v1 - v0) ^ (v2 - v0);
        ProjectTriToAxis(n, t1, t2, v0, v1, v2);
        ProjectToAxis(n, a1, a2);
        if (!RangesOverlap(t1, t2, a1, a2))
            return false;

        const cVec3 t[3] = { v1 - v0, v2 - v0, v2 - v1 };

        for (int i = 0; i < 3; ++i)
        {

            for (int j = 0; j < 3; ++j)
            {
                cVec3 axis = e[i] ^ t[j];
                float len = axis.LengthSqr();
                if (len <= 1e-4f)
                    continue; // Ignore tests on degenerate axes.

                ProjectTriToAxis(axis, t1, t2, v0, v1, v2);
                ProjectToAxis(axis, a1, a2);
                if (!RangesOverlap(t1, t2, a1, a2))
                    return false;
            }
        }

        // No separating axis exists, the AABB and triangle intersect.
        return true;
    }

    __forceinline bool Contains(const cVec3 &Point) const
    {
        return
            (mMin.x <= Point.x) && (mMax.x >= Point.x) &&
            (mMin.y <= Point.y) && (mMax.y >= Point.y) &&
            (mMin.z <= Point.z) && (mMax.z >= Point.z);
    }

    cAABB Transform(const cMat4& transform) const
    {
        cVec3 points[] = {
            { mMin.x, mMin.y, mMin.z },
            { mMax.x, mMin.y, mMin.z },
            { mMin.x, mMax.y, mMin.z },
            { mMax.x, mMax.y, mMin.z },
            { mMin.x, mMin.y, mMax.z },
            { mMax.x, mMin.y, mMax.z },
            { mMin.x, mMax.y, mMax.z },
            { mMax.x, mMax.y, mMax.z },
        };

        cAABB newAABB(FLT_MAX, -FLT_MAX);

        for (auto& point : points)
        {
            auto p = point * transform;
            newAABB.Add(p);
        }

        return newAABB;
    }
};


class cAABBInt2;

class cAABB2 : public cAABBImpl < cVec2 >
{
public:
    __forceinline cAABB2 ( ) { }
    __forceinline cAABB2(const cAABBInt2 & Other);
    __forceinline cAABB2(const cAABB2 & Other) : cAABBImpl(Other.mMin, Other.mMax) { }
    __forceinline cAABB2 ( cVec2 Min, cVec2 Max ) : cAABBImpl ( Min, Max ) { }
    
    __forceinline ~cAABB2() { }
};

class cAABBInt2 : public cAABBImpl < cVec2Int >
{
public:
    __forceinline cAABBInt2 ( cVec2Int Min, cVec2Int Max ) : cAABBImpl ( Min,  Max ) { }
    __forceinline cAABBInt2 ( const cAABB2 & other ) : cAABBImpl ( other.mMin, other.mMax ) {}
    __forceinline cAABBInt2 ( const cAABBInt2 & other ) : cAABBImpl ( other.mMin, other.mMax ) {}
    __forceinline cAABBInt2 ( ) {}
    __forceinline ~cAABBInt2() { }

    __forceinline int GetLargestDimention ( )
    {
        cVec2Int diff = mMax - mMin;
        return Utils::Max ( diff.x, diff.y );
    }

    __forceinline bool operator == ( const cAABBInt2 &other ) const  { return mMin == other.mMin && mMax == other.mMax; }
};

inline cAABB2::cAABB2( const cAABBInt2 & Other ) : cAABBImpl < cVec2 > ( Other.mMin, Other.mMax ) { }

class cAABBInt3 : public cAABBImpl < cVec3Int >
{
public:
    __forceinline cAABBInt3 ( cVec3Int Min, cVec3Int Max ) : cAABBImpl ( Min,  Max ) { }
    __forceinline cAABBInt3 ( const cAABB & other ) : cAABBImpl ( other.mMin, other.mMax ) {}
    __forceinline cAABBInt3 ( const cAABBInt3 & other ) : cAABBImpl ( other.mMin, other.mMax ) {}
    __forceinline cAABBInt3 ( ) {}
    __forceinline ~cAABBInt3() { }

    __forceinline int GetLargestDimention ( )
    {
        cVec3Int diff = mMax - mMin;
        return Utils::Max ( Utils::Max ( diff.x, diff.y ), diff.z );
    }

    __forceinline bool operator == (const cAABBInt3 &other) const { return mMin == other.mMin && mMax == other.mMax; }
    __forceinline void operator = (const cAABBInt3 &other) { mMin = other.mMin; mMax = other.mMax; }
};


#endif
