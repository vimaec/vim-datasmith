
void cTransform3::ToMat4 ( cMat4 &mat) const
{
	(~mOri).toMat4 ( mat );
	mat.mRow0 *= mScale.x;
	mat.mRow1 *= mScale.y;
	mat.mRow2 *= mScale.z;
	mat.SetTranslation ( mPos );
}

cTransform3 & cTransform3::Invert ( )
{
	mOri = ~mOri;
	mPos = -mPos;
	mScale = mScale.Reciprical();

	return *this;
}

void cTransform3::Inverted ( cTransform3 & T ) const
{
	T = *this;
	T.Invert();
}

void cTransform3::SetPos ( cVec3 Pos )
{
	mPos = Pos;
}

void cTransform3::SetOri(cQuat Ori)
{
	mOri = Ori;
}

void cTransform3::SetScale(cVec3 Scale)
{
	mScale = Scale;
}

cVec3 cTransform3::ToWorldPos(const cVec3 & Pos) const
{
	return ((~mOri) * (Pos % mScale)) + mPos;
}

cVec3 cTransform3::ToLocalPos(const cVec3 & Pos) const
{
	return ((mOri) * (Pos - mPos)) |  mScale;
}

cVec3 cTransform3::ToWorldDir(const cVec3 & Dir) const
{
	return (~mOri) * (Dir % mScale);
}

cVec3 cTransform3::ToLocalDir(const cVec3 & Dir) const
{
	return ((mOri) * Dir) | mScale;
}

void cTransform3::ToWorldAABB	( cAABB * AABB ) 		const
{
	cVec3 verts [ 8 ];
	cMat4 m;
	ToMat4 ( m );

	verts [ 0 ] = ( cVec3 ( AABB->mMin.x, AABB->mMin.y, AABB->mMin.z ) * m );
	verts [ 1 ] = ( cVec3 ( AABB->mMin.x, AABB->mMin.y, AABB->mMax.z ) * m );
	verts [ 2 ] = ( cVec3 ( AABB->mMin.x, AABB->mMax.y, AABB->mMin.z ) * m );
	verts [ 3 ] = ( cVec3 ( AABB->mMin.x, AABB->mMax.y, AABB->mMax.z ) * m );
	verts [ 4 ] = ( cVec3 ( AABB->mMax.x, AABB->mMin.y, AABB->mMin.z ) * m );
	verts [ 5 ] = ( cVec3 ( AABB->mMax.x, AABB->mMin.y, AABB->mMax.z ) * m );
	verts [ 6 ] = ( cVec3 ( AABB->mMax.x, AABB->mMax.y, AABB->mMin.z ) * m );
	verts [ 7 ] = ( cVec3 ( AABB->mMax.x, AABB->mMax.y, AABB->mMax.z ) * m );

	AABB->mMin.x = FLT_MAX;
	AABB->mMin.y = FLT_MAX;
	AABB->mMin.z = FLT_MAX;
	AABB->mMax.x = -FLT_MAX;
	AABB->mMax.y = -FLT_MAX;
	AABB->mMax.z = -FLT_MAX;

	for ( uint32_t i = 0; i < 8; i++ )
	{
		if ( verts [ i ].x > AABB->mMax.x ) AABB->mMax.x = verts [ i ].x;
		if ( verts [ i ].y > AABB->mMax.y ) AABB->mMax.y = verts [ i ].y;
		if ( verts [ i ].z > AABB->mMax.z ) AABB->mMax.z = verts [ i ].z;
		if ( verts [ i ].x < AABB->mMin.x ) AABB->mMin.x = verts [ i ].x;
		if ( verts [ i ].y < AABB->mMin.y ) AABB->mMin.y = verts [ i ].y;
		if ( verts [ i ].z < AABB->mMin.z ) AABB->mMin.z = verts [ i ].z;
	}
}


cTransform3 cTransform3::ToWorld(const cTransform3 & Trans) const
{
	cTransform3 trans;
	trans.mOri = (mOri * Trans.mOri);
	trans.mPos = ToWorldPos(Trans.mPos);
	trans.mScale = mScale % Trans.mScale;
	return trans;
}

cTransform3 cTransform3::ToLocal(const cTransform3 & Trans) const
{
	cTransform3 trans;
	cQuat invOri = ~mOri;
	trans.mOri = (invOri * Trans.mOri);
	trans.mPos = ToLocalPos(Trans.mPos);
	trans.mScale = Trans.mScale | mScale;
	return trans;
}

bool cTransform3::IsValid() const
{
	return true;
}

cTransform3::operator cMat4 () const
{
	cMat4 mat;
	this->ToMat4 ( mat );
	return mat;
}

cTransform3 cTransform3::Lerp(cTransform3 &First, cTransform3 &Second, float T)
{
	cTransform3 ret;
	ret.mPos = First.mPos * (1.0f - T) + Second.mPos * T;
	ret.mOri.mQuat = (First.mOri.mQuat * (1.0f - T) + Second.mOri.mQuat * T).Normalised();
	ret.mScale = (First.mScale * (1.0f - T) + Second.mScale * T);
	return ret;
}
