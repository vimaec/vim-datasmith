#ifndef __CTRANSFORM_H__
#define __CTRANSFORM_H__

#include "cMat.h"
#include "cQuat.h"
#include "cAABB.h"

class 
    cTransform3
{
public: // Members
    cQuat    mOri;
    cVec3    mPos;
    cVec3    mScale;

public: // Constructors
    __forceinline  cTransform3 ( )                           : mScale(1.0f, 1.0f, 1.0f) { }
    __forceinline  cTransform3 ( const cTransform3 & Other ) : mOri ( Other.mOri ), mPos ( Other.mPos ), mScale( Other.mScale) { }
    __forceinline  cTransform3 ( const cQuat & Ori )         : mOri ( Ori ), mScale(1.0f, 1.0f, 1.0f) { }
    __forceinline  cTransform3 ( const cVec3 & Pos )         : mPos ( Pos ), mScale(1.0f, 1.0f, 1.0f) { }
    __forceinline  cTransform3 ( const cQuat & Ori, const cVec3 &  Pos, const cVec3 &Scale = cVec3(1.0f, 1.0f, 1.0f) ) : mOri ( Ori ), mPos ( Pos ), mScale(Scale) { }

public: // Methods
    __forceinline  void          ToMat4      ( cMat4& ) const;
    __forceinline  void          Inverted    ( cTransform3& ) const;
    __forceinline  cTransform3 & Invert      ( );
    __forceinline  void          SetPos      ( cVec3 Pos );
    __forceinline  void          SetOri      ( cQuat Ori );
    __forceinline  void          SetScale    (cVec3 Scale);

    __forceinline  cVec3         ToWorldPos  (const cVec3 & Pos) const;
    __forceinline  cVec3         ToLocalPos  (const cVec3 & Pos) const;
    __forceinline  cVec3         ToWorldDir  (const cVec3 & Dir) const;
    __forceinline  cVec3         ToLocalDir  (const cVec3 & Dir) const;
    __forceinline  void          ToWorldAABB (cAABB * AABB ) const;
    __forceinline  cTransform3   ToWorld     (const cTransform3 & Trans) const;
    __forceinline  cTransform3   ToLocal     (const cTransform3 & Trans)const;
    __forceinline  bool          IsValid     () const;

    __forceinline static cTransform3 Lerp(cTransform3 &First, cTransform3 &Second, float T);

    __forceinline bool           operator == (const cTransform3& Other) { return mPos == Other.mPos && mOri == Other.mOri; }
    __forceinline bool           operator != (const cTransform3& Other) { return mPos != Other.mPos || mOri != Other.mOri; }

public: // Operators
    __forceinline  operator cMat4 () const;
};

#include "cTransform3.inl"

#endif