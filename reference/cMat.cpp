
#include "cVec.h"
#include "cMat.h"

cMat4 cMat4::Inverse ( ) const
{
    float m00 = m44[0][0], m01 = m44[0][1], m02 = m44[0][2], m03 = m44[0][3];
    float m10 = m44[1][0], m11 = m44[1][1], m12 = m44[1][2], m13 = m44[1][3];
    float m20 = m44[2][0], m21 = m44[2][1], m22 = m44[2][2], m23 = m44[2][3];
    float m30 = m44[3][0], m31 = m44[3][1], m32 = m44[3][2], m33 = m44[3][3];

    float v0 = m20 * m31 - m21 * m30;
    float v1 = m20 * m32 - m22 * m30;
    float v2 = m20 * m33 - m23 * m30;
    float v3 = m21 * m32 - m22 * m31;
    float v4 = m21 * m33 - m23 * m31;
    float v5 = m22 * m33 - m23 * m32;

    float t00 = + (v5 * m11 - v4 * m12 + v3 * m13);
    float t10 = - (v5 * m10 - v2 * m12 + v1 * m13);
    float t20 = + (v4 * m10 - v2 * m11 + v0 * m13);
    float t30 = - (v3 * m10 - v1 * m11 + v0 * m12);

    float invDet = 1 / (t00 * m00 + t10 * m01 + t20 * m02 + t30 * m03);

    float d00 = t00 * invDet;
    float d10 = t10 * invDet;
    float d20 = t20 * invDet;
    float d30 = t30 * invDet;

    float d01 = - (v5 * m01 - v4 * m02 + v3 * m03) * invDet;
    float d11 = + (v5 * m00 - v2 * m02 + v1 * m03) * invDet;
    float d21 = - (v4 * m00 - v2 * m01 + v0 * m03) * invDet;
    float d31 = + (v3 * m00 - v1 * m01 + v0 * m02) * invDet;

    v0 = m10 * m31 - m11 * m30;
    v1 = m10 * m32 - m12 * m30;
    v2 = m10 * m33 - m13 * m30;
    v3 = m11 * m32 - m12 * m31;
    v4 = m11 * m33 - m13 * m31;
    v5 = m12 * m33 - m13 * m32;

    float d02 = + (v5 * m01 - v4 * m02 + v3 * m03) * invDet;
    float d12 = - (v5 * m00 - v2 * m02 + v1 * m03) * invDet;
    float d22 = + (v4 * m00 - v2 * m01 + v0 * m03) * invDet;
    float d32 = - (v3 * m00 - v1 * m01 + v0 * m02) * invDet;

    v0 = m21 * m10 - m20 * m11;
    v1 = m22 * m10 - m20 * m12;
    v2 = m23 * m10 - m20 * m13;
    v3 = m22 * m11 - m21 * m12;
    v4 = m23 * m11 - m21 * m13;
    v5 = m23 * m12 - m22 * m13;

    float d03 = - (v5 * m01 - v4 * m02 + v3 * m03) * invDet;
    float d13 = + (v5 * m00 - v2 * m02 + v1 * m03) * invDet;
    float d23 = - (v4 * m00 - v2 * m01 + v0 * m03) * invDet;
    float d33 = + (v3 * m00 - v1 * m01 + v0 * m02) * invDet;

    return cMat4(
        d00, d01, d02, d03,
        d10, d11, d12, d13,
        d20, d21, d22, d23,
        d30, d31, d32, d33);
}


cMat4 cMat4::rotMatX(const float angle){
    float cosA = cosf(angle), sinA = sinf(angle);
    
    return cMat4(
        1, 0,     0,    0,
        0, cosA, -sinA, 0,
        0, sinA,  cosA, 0,
        0, 0,     0,    1);
}

cMat4 cMat4::rotMatY(const float angle){
    float cosA = cosf(angle), sinA = sinf(angle);

    return cMat4(
        cosA, 0, -sinA, 0,
        0,    1,  0,    0,
        sinA, 0,  cosA, 0,
        0,    0,  0,    1);
}

cMat4 cMat4::rotMatZ(const float angle){
    float cosA = cosf(angle), sinA = sinf(angle);

    return cMat4(
        cosA, -sinA, 0, 0,
        sinA,  cosA, 0, 0,
        0,     0,    1, 0,
        0,     0,    0, 1);
}

cMat4 cMat4::rotMatXY(const float angleX, const float angleY){
    float cosX = cosf(angleX), sinX = sinf(angleX), 
        cosY = cosf(angleY), sinY = sinf(angleY);

    return cMat4(
        cosY,        0,    -sinY,        0,
        -sinX * sinY, cosX, -sinX * cosY, 0,
        cosX * sinY, sinX,  cosX * cosY, 0,
        0,           0,     0,           1);
}

cMat4 cMat4::rotMatZXY(const float angleX, const float angleY, const float angleZ){
    float cosX = cosf(angleX), sinX = sinf(angleX), 
        cosY = cosf(angleY), sinY = sinf(angleY),
        cosZ = cosf(angleZ), sinZ = sinf(angleZ);

    return cMat4(
        cosY * cosZ + sinX * sinY * sinZ,   -cosX * sinZ,    sinX * cosY * sinZ - sinY * cosZ,  0,
        cosY * sinZ - sinX * sinY * cosZ,    cosX * cosZ,   -sinY * sinZ - sinX * cosY * cosZ,  0,
        cosX * sinY,                         sinX,           cosX * cosY,                       0,
        0,                                   0,              0,                                 1);
}

cMat4 cMat4::transMat(const cVec4 &v)
{
    return cMat4 (
        1.0f, 0.0f, 0.0f, v.x, 
        0.0f, 1.0f, 0.0f, v.y, 
        0.0f, 0.0f, 1.0f, v.z, 
        0.0f, 0.0f, 0.0f, 1.0f);
}

cMat4 cMat4::transMat(const float x, const float y, const float z)
{
    return cMat4 (
        1.0f, 0.0f, 0.0f, x, 
        0.0f, 1.0f, 0.0f, y, 
        0.0f, 0.0f, 1.0f, z, 
        0.0f, 0.0f, 0.0f, 1.0f);
}

cMat4 cMat4::scaleMat(const float x, const float y, const float z){
    return cMat4(x,0,0,0, 0,y,0,0, 0,0,z,0, 0,0,0,1);
}

cMat4 cMat4::perspProjMat(const float fov, const float zNear, const float zFar){
    float s = cosf(0.5f * fov) / sinf(0.5f * fov);

    return cMat4(
        s, 0, 0, 0,
        0, s, 0, 0,
        0, 0, (zFar + zNear) / (zFar - zNear), -(2 * zFar * zNear) / (zFar - zNear),
        0, 0, 1, 0);
}

cMat4 cMat4::perspProjMatX(const float fov, const float AspectRatio, const float zNear, const float zFar)
{
    //           float yScale = cosf(0.5f * fov) / sinf(0.5f * fov);
    //           float xScale = yScale / AspectRatio;
    //       
    //           return cMat4(
    //               xScale,        0,        0, 0,
    //               0,            yScale, 0, 0,
    //               0,            0,        zFar / (zFar - zNear), 1,
    //               0,            0,        -zNear*zFar / (zFar - zNear), 0);      
//     cMat4 vulkanCorrection(
//         1, 0, 0, 0,
//         0, 1, 0, 0,
//         0, 0, 0.5f, 0.5f,
//         0, 0, 0, 1
//     );
// 
//     const float f = 0.5f * fov;
//     const float h = cosf(f) / sinf(f);
//     const float w = h / AspectRatio;
//     return cMat4(
//         w, 0, 0, 0,
//         0, h, 0, 0,
//         0, 0, zFar / (zFar - zNear), -(zFar * zNear) / (zFar - zNear),
//         0, 0, 1, 0
//     );

    float f = 1.0f / tanf(0.5f * fov);
    return cMat4(
      f / AspectRatio, 0.0f, 0.0f,0.0f,
      0.0f, -f,   0.0f, 0.0f,
      0.0f, 0.0f, zFar / (zNear - zFar), -1.0f,
      0.0f, 0.0f, (zNear * zFar) / (zNear - zFar), 0.0f
    ).Transposed();
}

cMat4 cMat4::perspProjMatY(const float fov, const int width, const int height, const float zNear, const float zFar){
    float h = tanf(0.5f * fov);
    float w = (h * width) / height;

    return cMat4(
        1.0f / w, 0,        0, 0,
        0,        1.0f / h, 0, 0,
        0,        0,        (zFar + zNear) / (zFar - zNear), -(2 * zFar * zNear) / (zFar - zNear),
        0,        0,        1, 0);
}

cMat4 cMat4::orthoProjMatX(const float left, const float right, const float top, const float bottom, const float zNear, const float zFar){
    const float rl = right - left;
    const float tb = top - bottom;
    const float fn = zFar - zNear;

/*        return cMat4(
        2.0f / rl, 0,         0,         -(right + left) / rl,
        0,         2.0f / tb, 0,         -(top + bottom) / tb,
        0,         0,        -2.0f / fn, -(zFar + zNear) / fn,
        0,         0,         0,         1);*/
    return cMat4(
        2.0f / rl, 0,            0,            -(right + left) / rl,
        0,         2.0f / tb,    0,            -(top + bottom) / tb,
        0,         0,            1.0f / fn,    -(zFar + zNear) / fn,
        0,         0,            0,            1);
}

cMat4 cMat4::cubeViewMat(const unsigned int side){
    switch(side){
    case POSITIVE_X:
        return cMat4(
            0, 0, -1, 0,
            0, 1,  0, 0,
            1, 0,  0, 0,
            0, 0,  0, 1);
    case NEGATIVE_X:
        return cMat4(
            0, 0, 1, 0,
            0, 1, 0, 0,
            -1, 0, 0, 0,
            0, 0, 0, 1);
    case POSITIVE_Y:
        return cMat4(
            1, 0,  0, 0,
            0, 0, -1, 0,
            0, 1,  0, 0,
            0, 0,  0, 1);
    case NEGATIVE_Y:
        return cMat4(
            1,  0, 0, 0,
            0,  0, 1, 0,
            0, -1, 0, 0,
            0,  0, 0, 1);
    case POSITIVE_Z:
        return cMat4(
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1);
    //case NEGATIVE_Z:
    default:
        return cMat4(
            -1, 0,  0, 0,
            0, 1,  0, 0,
            0, 0, -1, 0,
            0, 0,  0, 1);
    }
}

cMat4 cMat4::cubeProjMatGL(const float zNear, const float zFar){
    return cMat4(
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, -1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, (zFar + zNear) / (zFar - zNear), -(2 * zFar * zNear) / (zFar - zNear),
        0.0f, 0.0f, 1.0f, 0.0f);// * scale(1, -1, 1);
}

cMat4 cMat4::cubeProjMatD3D(const float zNear, const float zFar){
    return cMat4(
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, (zFar + zNear) / (zFar - zNear), -(2 * zFar * zNear) / (zFar - zNear),
        0.0f, 0.0f, 1.0f, 0.0f);
}

cMat4 cMat4::identMat(){
    return cMat4(1.0f,0.0f,0.0f,0.0f, 0.0f,1.0f,0.0f,0.0f, 0.0f,0.0f,1.0f,0.0f, 0.0f,0.0f,0.0f,1.0f);
}

cMat4 cMat4::LookAt ( const cVec3 & Direction, const cVec3 &Up)
{
    cVec3 direction = Direction.Normalised();
    cVec3 side = Up ^ direction;
    cVec3 up = direction ^ side;

    return cMat4 ( 
        cVec4 ( side, 0.0f ),
        cVec4 ( up, 0.0f ),
        cVec4 ( direction, 0.0f ),
        cVec4 ( 0.0f, 0.0f, 0.0f, 1.0f )
        );
}