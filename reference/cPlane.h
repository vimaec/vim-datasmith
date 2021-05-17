#ifndef __CPLANE_H__
#define __CPLANE_H__

#include "cVec.h"
//#include "cRay.h"

class cPlane 
{
public:
    cVec3 mNormal;
    float mDistance;

    cPlane ( ) { }

    cPlane (const cVec3 &Normal, const cVec3 &Point) :
        mNormal(Normal),
        mDistance(Point * Normal)
    {
    }

    float GetDistance (const cVec3 &point )
    {
        return point * mNormal - mDistance;
    }

    void CreateFrom3Points (const cVec3 & v0, const cVec3 & v1, const cVec3 & v2 )
    {
        mNormal = (v0 - v1) ^ (v2 - v1);
        mNormal.Normalise();

        mDistance = mNormal * v1;
    }

    void CreateFromNormalAndPoint (const cVec3 & Normal, const cVec3 & Point )
    {
        mNormal = Normal;
        mNormal.Normalise();

        mDistance = mNormal * Point;
    }

    static cPlane From3Points (const cVec3 & v0, const cVec3 & v1, const cVec3 & v2 )
    {
        cPlane plane;
        plane.CreateFrom3Points( v0, v1, v2 );
        return plane;
    }

    static cPlane FromNormalAndPoint (const cVec3 & Normal, const cVec3 & Point )
    {
        cPlane plane;
        plane.CreateFromNormalAndPoint( Normal, Point );
        return plane;
    }

    int SideOfPlane (const cVec3 & Point ) const
    {
         cVec3 temp = Point - mNormal * mDistance;
         return temp * mNormal > 0;
    }

//     float Intersection(const cRay &Ray)
//     {
//         cVec3 p0 = mNormal * mDistance;
//         float d = (p0 - Ray.mOrigin) * mNormal / (Ray.mDirection * mNormal);
//         return d;
//     }

    cVec3 ProjectPointOntoPlane(const cVec3 & Point)
    {
        float dist = Point * mNormal - mDistance;
        return Point - mNormal * dist;
    }
};


#endif