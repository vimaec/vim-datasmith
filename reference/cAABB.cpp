#include "cAABB.h"
#include "Utils/Utils.h"


cVec2 GetMinMax(float f0, float f1, float f2)
{
    cVec2 ret;
    ret.x = Utils::Min(Utils::Min(f0, f1), f2);
    ret.y = Utils::Max(Utils::Max(f0, f1), f2);
    return ret;
}

cVec2 GetMinMax(float f0, float f1, float f2, float f3, float f4, float f5, float f6, float f7)
{
    cVec2 ret;
    ret.x = Utils::Min(Utils::Min(Utils::Min(Utils::Min(Utils::Min(Utils::Min(Utils::Min(f0, f1), f2), f3), f4), f5), f6), f7);
    ret.y = Utils::Max(Utils::Max(Utils::Max(Utils::Max(Utils::Max(Utils::Max(Utils::Max(f0, f1), f2), f3), f4), f5), f6), f7);
    return ret;
}

bool cAABB::Intersects(const cVec3 & v0, const cVec3 & v1, const cVec3 & v2) const
{
    cVec2 tMinMax;

    // Test the 3 aabb axi for separation
    tMinMax = GetMinMax(v0.x, v1.x, v2.x);
    if (tMinMax.x > mMax.x || tMinMax.y < mMin.x)
    {
        return false;
    }    
    
    tMinMax = GetMinMax(v0.y, v1.y, v2.y);
    if (tMinMax.x > mMax.y || tMinMax.y < mMin.y)
    {
        return false;
    }    
    
    tMinMax = GetMinMax(v0.z, v1.z, v2.z);
    if (tMinMax.x > mMax.z || tMinMax.y < mMin.z)
    {
        return false;
    }

    // Test the triangle plane for separation
    cVec3 normal = ((v0 - v1) ^ (v0 - v2)).Normalised();
    float tp0 = normal * v0;

    float bp0 = normal * cVec3(mMin.x, mMin.y, mMin.z);
    float bp1 = normal * cVec3(mMax.x, mMin.y, mMin.z);
    float bp2 = normal * cVec3(mMin.x, mMax.y, mMin.z);
    float bp3 = normal * cVec3(mMax.x, mMax.y, mMin.z);
    float bp4 = normal * cVec3(mMin.x, mMin.y, mMax.z);
    float bp5 = normal * cVec3(mMax.x, mMin.y, mMax.z);
    float bp6 = normal * cVec3(mMin.x, mMax.y, mMax.z);
    float bp7 = normal * cVec3(mMax.x, mMax.y, mMax.z);

    cVec2 bMinMax = GetMinMax(bp0, bp1, bp2, bp3, bp4, bp5, bp6, bp7);

    if (tp0 > bMinMax.y || tp0 < bMinMax.x)
    {
        return false;
    }

    return true;
}
// 
// bool cAABB::Intersects(const cRay & Ray) const
// {
//     float tmin = (mBounds[Ray.mSign[0]].x - Ray.mOrigin.x) * Ray.mInvDirection.x;
//     float tmax = (mBounds[1 - Ray.mSign[0]].x - Ray.mOrigin.x) * Ray.mInvDirection.x;
//     float tymin = (mBounds[Ray.mSign[1]].y - Ray.mOrigin.y) * Ray.mInvDirection.y;
//     float tymax = (mBounds[1 - Ray.mSign[1]].y - Ray.mOrigin.y) * Ray.mInvDirection.y;
//     float tzmin = (mBounds[Ray.mSign[2]].z - Ray.mOrigin.z) * Ray.mInvDirection.z;
//     float tzmax = (mBounds[1 - Ray.mSign[2]].z - Ray.mOrigin.z) * Ray.mInvDirection.z;
// 
//     if ((tmin > tymax) || (tymin > tmax))
//     {
//         return false;
//     }
// 
//     if (tymin > tmin)
//     {
//         tmin = tymin;
//     }
// 
//     if (tymax < tmax)
//     {
//         tmax = tymax;
//     }
// 
//     if ((tmin > tzmax) || (tzmin > tmax))
//     {
//         return false;
//     }
// 
//     if (tzmin > tmin)
//     {
//         tmin = tzmin;
//     }
// 
//     if (tzmax < tmax)
//     {
//         tmax = tzmax;
//     }
// 
//     //        return ( (tmin < t1) && (tmax > t0) );
//     Ray.mMin = tmin;
//     Ray.mMax = tmax;
//     return tmax > 0.0f;
// }