
inline cMat3 cMat3::Transposed(void) const {
	cMat3 ret(
		m33[0][0], m33[1][0], m33[2][0],
		m33[0][1], m33[1][1], m33[2][1],
		m33[0][2], m33[1][2], m33[2][2]);

	return ret;
}


inline cMat4 cMat4::ToD3DProjection(void) {
	cMat4 mat;

	mat.m16[0]	= m16[0];	mat.m16[1]	= m16[1];	mat.m16[2]	= m16[2];	mat.m16[3]	= m16[3];
	mat.m16[4]	= m16[4];	mat.m16[5]	= m16[5];	mat.m16[6]	= m16[6];	mat.m16[7]	= m16[7];
	mat.m16[8]	= (m16[8] + m16[12]) * 0.5f;	mat.m16[9]	= (m16[9] + m16[13]) * 0.5f;	mat.m16[10] = (m16[10] + m16[14]) * 0.5f;	mat.m16[11] = (m16[11] + m16[15]) * 0.5f;
	mat.m16[12] = m16[12];	mat.m16[13] = m16[13];	mat.m16[14] = m16[14];	mat.m16[15] = m16[15];

	return mat;
}

inline cMat4 cMat4::Transposed(void) const {
	cMat4 ret(
		m44[0][0], m44[1][0], m44[2][0], m44[3][0],
		m44[0][1], m44[1][1], m44[2][1], m44[3][1],
		m44[0][2], m44[1][2], m44[2][2], m44[3][2],
		m44[0][3], m44[1][3], m44[2][3], m44[3][3]);

	return ret;
}

inline void cMat4::TranslateLocal(const cVec3 &v){
	m44[0][3] += ((cVec3&)m44[0]) * v;
	m44[1][3] += ((cVec3&)m44[1]) * v;
	m44[2][3] += ((cVec3&)m44[2]) * v;
	m44[3][3] += ((cVec3&)m44[3]) * v;
}

inline void cMat4::TranslateLocal(const cVec4 &v){
	const cVec4 t(v.x, v.y, v.z, 0.0f);
	m44[0][3] += mRow[0] * t;
	m44[1][3] += mRow[1] * t;
	m44[2][3] += mRow[2] * t;
	m44[3][3] += mRow[3] * t;
}

inline void cMat4::SetTranslation(const cVec4 &v)
{
	m44[0][3] = v.x;
	m44[1][3] = v.y;
	m44[2][3] = v.z;
	m44[3][3] = 1.0f;
}

inline void cMat4::SetTranslation(const cVec3 &v)
{
	mRow3 = v;
	//m44[0][3] = v.x;
	//m44[1][3] = v.y;
	//m44[2][3] = v.z;
	//m44[3][3] = 1.0f;
}

inline cMat4 operator + (const cMat4 &m, const cMat4 &n){
	cMat4 ret(
		m.m44[0][0] + n.m44[0][0], m.m44[1][0] + n.m44[1][0], m.m44[2][0] + n.m44[2][0], m.m44[3][0] + n.m44[3][0],
		m.m44[0][1] + n.m44[0][1], m.m44[1][1] + n.m44[1][1], m.m44[2][1] + n.m44[2][1], m.m44[3][1] + n.m44[3][1],
		m.m44[0][2] + n.m44[0][2], m.m44[1][2] + n.m44[1][2], m.m44[2][2] + n.m44[2][2], m.m44[3][2] + n.m44[3][2],
		m.m44[0][3] + n.m44[0][3], m.m44[1][3] + n.m44[1][3], m.m44[2][3] + n.m44[2][3], m.m44[3][3] + n.m44[3][3]
	);

	return ret;
}

inline cMat4 operator - (const cMat4 &m, const cMat4 &n){
	cMat4 ret(
		m.m44[0][0] - n.m44[0][0], m.m44[1][0] - n.m44[1][0], m.m44[2][0] - n.m44[2][0], m.m44[3][0] - n.m44[3][0],
		m.m44[0][1] - n.m44[0][1], m.m44[1][1] - n.m44[1][1], m.m44[2][1] - n.m44[2][1], m.m44[3][1] - n.m44[3][1],
		m.m44[0][2] - n.m44[0][2], m.m44[1][2] - n.m44[1][2], m.m44[2][2] - n.m44[2][2], m.m44[3][2] - n.m44[3][2],
		m.m44[0][3] - n.m44[0][3], m.m44[1][3] - n.m44[1][3], m.m44[2][3] - n.m44[2][3], m.m44[3][3] - n.m44[3][3]
	);

	return ret;
}

inline cMat4 operator - (const cMat4 &n){
	cMat4 ret(
		- n.m44[0][0], - n.m44[1][0], - n.m44[2][0], - n.m44[3][0],
		- n.m44[0][1], - n.m44[1][1], - n.m44[2][1], - n.m44[3][1],
		- n.m44[0][2], - n.m44[1][2], - n.m44[2][2], - n.m44[3][2],
		- n.m44[0][3], - n.m44[1][3], - n.m44[2][3], - n.m44[3][3]
	);

	return ret;
}

#define rcDot4(r, c) (m.m44[r][0] * n.m44[0][c] + m.m44[r][1] * n.m44[1][c] + m.m44[r][2] * n.m44[2][c] + m.m44[r][3] * n.m44[3][c])

inline cMat4 operator * (const cMat4 &m, const cMat4 &n){
	cMat4 ret(
		rcDot4(0, 0), rcDot4(0, 1), rcDot4(0, 2), rcDot4(0, 3),
		rcDot4(1, 0), rcDot4(1, 1), rcDot4(1, 2), rcDot4(1, 3),
		rcDot4(2, 0), rcDot4(2, 1), rcDot4(2, 2), rcDot4(2, 3),
		rcDot4(3, 0), rcDot4(3, 1), rcDot4(3, 2), rcDot4(3, 3)
	);

	return ret;
}

inline cVec4 operator * (const cMat4 &m, const cVec4 &v)
{
	return cVec4(m.mRow0 * v, m.mRow1 * v, m.mRow2 * v, m.mRow3 * v);
}

inline cVec4 operator * (const cVec4 &v, const cMat4 &m)
{
	cMat4 mt = m.Transposed();
	return cVec4(mt.mRow0 * v, mt.mRow1 * v, mt.mRow2 * v, mt.mRow3 * v);
}

inline cMat4 operator * (const cMat4 &m, const float x){
	cMat4 ret(m.mRow0 * x, m.mRow1 * x, m.mRow2 * x, m.mRow3 * x);

	return ret;
}


inline cVec3 operator * (const cMat3 &m, const cVec3 &v)
{
	return cVec3(m.mRow0 * v, m.mRow1 * v, m.mRow2 * v);
}

inline cVec3 operator * (const cVec3 &v, const cMat3 &m)
{
	cMat3 mt = m.Transposed();
	return cVec3(mt.mRow0 * v, mt.mRow1 * v, mt.mRow2 * v);
}

inline cMat4 operator ! (const cMat4 &m){
	cMat4 mat;

	float p00 = m.m44[2][2] * m.m44[3][3];
	float p01 = m.m44[3][2] * m.m44[2][3];
	float p02 = m.m44[1][2] * m.m44[3][3];
	float p03 = m.m44[3][2] * m.m44[1][3];
	float p04 = m.m44[1][2] * m.m44[2][3];
	float p05 = m.m44[2][2] * m.m44[1][3];
	float p06 = m.m44[0][2] * m.m44[3][3];
	float p07 = m.m44[3][2] * m.m44[0][3];
	float p08 = m.m44[0][2] * m.m44[2][3];
	float p09 = m.m44[2][2] * m.m44[0][3];
	float p10 = m.m44[0][2] * m.m44[1][3];
	float p11 = m.m44[1][2] * m.m44[0][3];

	mat.m44[0][0] = (p00 * m.m44[1][1] + p03 * m.m44[2][1] + p04 * m.m44[3][1]) - (p01 * m.m44[1][1] + p02 * m.m44[2][1] + p05 * m.m44[3][1]);
	mat.m44[0][1] = (p01 * m.m44[0][1] + p06 * m.m44[2][1] + p09 * m.m44[3][1]) - (p00 * m.m44[0][1] + p07 * m.m44[2][1] + p08 * m.m44[3][1]);
	mat.m44[0][2] = (p02 * m.m44[0][1] + p07 * m.m44[1][1] + p10 * m.m44[3][1]) - (p03 * m.m44[0][1] + p06 * m.m44[1][1] + p11 * m.m44[3][1]);
	mat.m44[0][3] = (p05 * m.m44[0][1] + p08 * m.m44[1][1] + p11 * m.m44[2][1]) - (p04 * m.m44[0][1] + p09 * m.m44[1][1] + p10 * m.m44[2][1]);
	mat.m44[1][0] = (p01 * m.m44[1][0] + p02 * m.m44[2][0] + p05 * m.m44[3][0]) - (p00 * m.m44[1][0] + p03 * m.m44[2][0] + p04 * m.m44[3][0]);
	mat.m44[1][1] = (p00 * m.m44[0][0] + p07 * m.m44[2][0] + p08 * m.m44[3][0]) - (p01 * m.m44[0][0] + p06 * m.m44[2][0] + p09 * m.m44[3][0]);
	mat.m44[1][2] = (p03 * m.m44[0][0] + p06 * m.m44[1][0] + p11 * m.m44[3][0]) - (p02 * m.m44[0][0] + p07 * m.m44[1][0] + p10 * m.m44[3][0]);
	mat.m44[1][3] = (p04 * m.m44[0][0] + p09 * m.m44[1][0] + p10 * m.m44[2][0]) - (p05 * m.m44[0][0] + p08 * m.m44[1][0] + p11 * m.m44[2][0]);

	float q00 = m.m44[2][0] * m.m44[3][1];
	float q01 = m.m44[3][0] * m.m44[2][1];
	float q02 = m.m44[1][0] * m.m44[3][1];
	float q03 = m.m44[3][0] * m.m44[1][1];
	float q04 = m.m44[1][0] * m.m44[2][1];
	float q05 = m.m44[2][0] * m.m44[1][1];
	float q06 = m.m44[0][0] * m.m44[3][1];
	float q07 = m.m44[3][0] * m.m44[0][1];
	float q08 = m.m44[0][0] * m.m44[2][1];
	float q09 = m.m44[2][0] * m.m44[0][1];
	float q10 = m.m44[0][0] * m.m44[1][1];
	float q11 = m.m44[1][0] * m.m44[0][1];

	mat.m44[2][0] = (q00 * m.m44[1][3] + q03 * m.m44[2][3] + q04 * m.m44[3][3]) - (q01 * m.m44[1][3] + q02 * m.m44[2][3] + q05 * m.m44[3][3]);
	mat.m44[2][1] = (q01 * m.m44[0][3] + q06 * m.m44[2][3] + q09 * m.m44[3][3]) - (q00 * m.m44[0][3] + q07 * m.m44[2][3] + q08 * m.m44[3][3]);
	mat.m44[2][2] = (q02 * m.m44[0][3] + q07 * m.m44[1][3] + q10 * m.m44[3][3]) - (q03 * m.m44[0][3] + q06 * m.m44[1][3] + q11 * m.m44[3][3]);
	mat.m44[2][3] = (q05 * m.m44[0][3] + q08 * m.m44[1][3] + q11 * m.m44[2][3]) - (q04 * m.m44[0][3] + q09 * m.m44[1][3] + q10 * m.m44[2][3]);
	mat.m44[3][0] = (q02 * m.m44[2][2] + q05 * m.m44[3][2] + q01 * m.m44[1][2]) - (q04 * m.m44[3][2] + q00 * m.m44[1][2] + q03 * m.m44[2][2]);
	mat.m44[3][1] = (q08 * m.m44[3][2] + q00 * m.m44[0][2] + q07 * m.m44[2][2]) - (q06 * m.m44[2][2] + q09 * m.m44[3][2] + q01 * m.m44[0][2]);
	mat.m44[3][2] = (q06 * m.m44[1][2] + q11 * m.m44[3][2] + q03 * m.m44[0][2]) - (q10 * m.m44[3][2] + q02 * m.m44[0][2] + q07 * m.m44[1][2]);
	mat.m44[3][3] = (q10 * m.m44[2][2] + q04 * m.m44[0][2] + q09 * m.m44[1][2]) - (q08 * m.m44[1][2] + q11 * m.m44[2][2] + q05 * m.m44[0][2]);

	return mat * (1.0f / (m.m44[0][0] * mat.m44[0][0] + m.m44[1][0] * mat.m44[0][1] + m.m44[2][0] * mat.m44[0][2] + m.m44[3][0] * mat.m44[0][3]));
}
