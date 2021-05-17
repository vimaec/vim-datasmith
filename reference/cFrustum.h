#ifndef __CFRUSTUM_H__
#define __CFRUSTUM_H__

#include "cVec.h"
#include "cMat.h"
#include "cPlane.h"
#include "cAABB.h"

enum eFrustumPlane
{
    eFrustumPlane_Near = 0,
    eFrustumPlane_Far,
    eFrustumPlane_Top,
    eFrustumPlane_Bottom,
    eFrustumPlane_Left,
    eFrustumPlane_Right
};

class cFrustum
{
public:
    cPlane mPlanes[6];

    cFrustum ( ) {}
    cFrustum ( const cMat4 & ViewProjection )
    {
        CreateFromTP ( ViewProjection );
    }

    void CreateFromTP ( const cMat4 & ViewProjection )
    {
        cMat4 inverseVP = ViewProjection.Inverse();

        cVec4 _ppp ( 1.0f,  1.0f,  1.0f, 1.0f);
        cVec4 _ppn ( 1.0f,  1.0f, -1.0f, 1.0f);
        cVec4 _pnp ( 1.0f, -1.0f,  1.0f, 1.0f);
        cVec4 _pnn ( 1.0f, -1.0f, -1.0f, 1.0f);
        cVec4 _npp (-1.0f,  1.0f,  1.0f, 1.0f);
        cVec4 _npn (-1.0f,  1.0f, -1.0f, 1.0f);
        cVec4 _nnp (-1.0f, -1.0f,  1.0f, 1.0f);
        cVec4 _nnn (-1.0f, -1.0f, -1.0f, 1.0f);

        _ppp = _ppp * inverseVP;
        _ppn = _ppn * inverseVP;
        _pnp = _pnp * inverseVP;
        _pnn = _pnn * inverseVP;
        _npp = _npp * inverseVP;
        _npn = _npn * inverseVP;
        _nnp = _nnp * inverseVP;
        _nnn = _nnn * inverseVP;

        _ppp = _ppp / _ppp.w;
        _ppn = _ppn / _ppn.w;
        _pnp = _pnp / _pnp.w;
        _pnn = _pnn / _pnn.w;
        _npp = _npp / _npp.w;
        _npn = _npn / _npn.w;
        _nnp = _nnp / _nnp.w;
        _nnn = _nnn / _nnn.w;

        mPlanes [ eFrustumPlane_Near ].CreateFrom3Points ( _npn, _nnn, _pnn);
        mPlanes [ eFrustumPlane_Far ].CreateFrom3Points ( _ppp, _pnp, _nnp);
        mPlanes [ eFrustumPlane_Top ].CreateFrom3Points ( _npn, _ppn, _ppp );
        mPlanes [ eFrustumPlane_Bottom ].CreateFrom3Points ( _pnp, _pnn, _nnn);
        mPlanes [ eFrustumPlane_Left ].CreateFrom3Points ( _npp, _nnp, _nnn );
        mPlanes [ eFrustumPlane_Right ].CreateFrom3Points ( _ppn, _pnn, _pnp);

//         glm::vec4 x = glm::row(m, 0);
//         glm::vec4 y = glm::row(m, 1);
//         glm::vec4 z = glm::row(m, 2);
//         glm::vec4 w = glm::row(m, 3);
// 
//         right_ = w - x;
//         left_ = w + x;
//         top_ = w - y;
//         bottom_ = w + y;
//         far_ = w - z;
//         near_ = w + z;
    }

    bool Overlaps(const cAABB &AABB) const
    {
        return AABBTest(AABB) <= 0;
    }

    bool Constains(const cAABB &AABB) const
    {
        return AABBTest(AABB) < 0;
    }

    bool Overlaps(const cSphere &AABB) const
    {
        return SphereTest(AABB) <= 0;
    }

    bool Constains(const cSphere &AABB) const
    {
        return SphereTest(AABB) < 0;
    }

    // tests if a sphere is within the frustum
    int SphereTest(const cSphere& Sphere) const
    {
        // various distances
        float fDistance;

        // calculate our distances to each of the planes
        for (int i = 0; i < 6; ++i)
        {

            // find the distance to this plane
            fDistance = -mPlanes[i].mNormal * Sphere.mPos + mPlanes[i].mDistance;

            // else if the distance is between +- radius, then we intersect
            if ((float)fabs(fDistance) < Sphere.mRadius)
            {
                return 0;
            }
            // if this distance is < -sphere.radius, we are outside
            if (fDistance < -Sphere.mRadius)
            {
                return 1;
            }

        }

        // otherwise we are fully in view
        return -1;
    }

    int AABBTest ( const cAABB & AABB ) const
    {
        cVec3 corners[8];
        int totalIn = 0;

        // get the corners of the box into the vCorner array
        corners [ 0 ] = cVec3 ( AABB.mMin.x, AABB.mMin.y, AABB.mMin.z );
        corners [ 1 ] = cVec3 ( AABB.mMin.x, AABB.mMin.y, AABB.mMax.z );
        corners [ 2 ] = cVec3 ( AABB.mMin.x, AABB.mMax.y, AABB.mMin.z );
        corners [ 3 ] = cVec3 ( AABB.mMin.x, AABB.mMax.y, AABB.mMax.z );
        corners [ 4 ] = cVec3 ( AABB.mMax.x, AABB.mMin.y, AABB.mMin.z );
        corners [ 5 ] = cVec3 ( AABB.mMax.x, AABB.mMin.y, AABB.mMax.z );
        corners [ 6 ] = cVec3 ( AABB.mMax.x, AABB.mMax.y, AABB.mMin.z );
        corners [ 7 ] = cVec3 ( AABB.mMax.x, AABB.mMax.y, AABB.mMax.z );

        // test all 8 corners against the 6 sides 
        // if all points are behind 1 specific plane, we are out
        // if we are in with all points, then we are fully in
        for ( int p = 0; p < 6; ++p )
        {
            int inCount = 8;
            int pointIn = 1;

            for(int i = 0; i < 8; ++i)
            {
                // test this point against the planes
                if ( mPlanes[p].SideOfPlane ( corners[i] ) > 0 )
                {
                    pointIn = 0;
                    inCount--;
                }
            }

            // were all the points outside of plane p?
            if ( inCount == 0)
            {
                return 1;
            }

            // check if they were all on the right side of the plane
            totalIn += pointIn;
        }

        // so if iTotalIn is 6, then all are inside the view
        if(totalIn == 6)
        {
            return -1;
        }

        // we must be partly in then otherwise
        return 0;
    }
};

#endif