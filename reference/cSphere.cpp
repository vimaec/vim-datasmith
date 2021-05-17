#include "cSphere.h"
#include "cAABB.h"

cSphere::cSphere(const cVec3 &P0, const cVec3 &P1, const cVec3 &P2)
{
    cSphere s(P0, P1);
    float l = (s.mPos - P2).LengthSqr();
    if (l <= s.mRadius * s.mRadius + mRadiusEpsilon)
    {
        mPos = s.mPos;
        mRadius = s.mRadius;
        return;
    }

    s = cSphere(P1, P2);
    l = (s.mPos - P0).LengthSqr();
    if (l <= s.mRadius * s.mRadius + mRadiusEpsilon)
    {
        mPos = s.mPos;
        mRadius = s.mRadius;
        return;
    }

    s = cSphere(P2, P0);
    l = (s.mPos - P1).LengthSqr();
    if (l <= s.mRadius * s.mRadius + mRadiusEpsilon)
    {
        mPos = s.mPos;
        mRadius = s.mRadius;
        return;
    }

    cVec3 a = P1 - P0;
    cVec3 b = P2 - P0;

    float Denominator = 2.0f * ((a ^ b) * (a ^ b));

    cVec3 o =
        (((a ^ b) ^ a)*(b * b) +
            (b ^ (a ^ b)) * (a * a)) / Denominator;

    mRadius = o.Length() + mRadiusEpsilon;
    mPos = P0 + o;
}

cSphere::cSphere(const cVec3 &O, const cVec3 &A, const cVec3 &B, const cVec3 &C)
{
    cSphere s(O, A, B);
    float l = (s.mPos - C).LengthSqr();
    if (l <= s.mRadius * s.mRadius + mRadiusEpsilon)
    {
        mPos = s.mPos;
        mRadius = s.mRadius;
        return;
    }

    s = cSphere(O, A, C);
    l = (s.mPos - B).LengthSqr();
    if (l <= s.mRadius * s.mRadius + mRadiusEpsilon)
    {
        mPos = s.mPos;
        mRadius = s.mRadius;
        return;
    }

    s = cSphere(O, B, C);
    l = (s.mPos - A).LengthSqr();
    if (l <= s.mRadius * s.mRadius + mRadiusEpsilon)
    {
        mPos = s.mPos;
        mRadius = s.mRadius;
        return;
    }

    s = cSphere(A, B, C);
    l = (s.mPos - O).LengthSqr();
    if (l <= s.mRadius * s.mRadius + mRadiusEpsilon)
    {
        mPos = s.mPos;
        mRadius = s.mRadius;
        return;
    }

    cVec3 a = A - O;
    cVec3 b = B - O;
    cVec3 c = C - O;

    cMat3 matrix(a, b, c);

    float Denominator = 2.0f * matrix.Determenent();

    cVec3 o = ((c ^ 2) * (a ^ b) +
        (b ^ 2) * (c ^ a) +
        (a ^ 2) * (b ^ c)) / Denominator;

    mRadius = o.Length() + mRadiusEpsilon;
    mPos = O + o;
}

cAABB cSphere::GetAABB()
{
    return cAABB(mPos - cVec3(mRadius), mPos + cVec3(mRadius));
}
