#ifndef __MATRIX_H__
#define __MATRIX_H__

#include "cVec.h"
#include "MathConst.h"

#define POSITIVE_X 0
#define NEGATIVE_X 1
#define POSITIVE_Y 2
#define NEGATIVE_Y 3
#define POSITIVE_Z 4
#define NEGATIVE_Z 5

class 
    cMat3
{
public:
    union {
        struct {
            cVec3 mRow[3];
        };
        struct {
            cVec3 mRow0;
            cVec3 mRow1;
            cVec3 mRow2;
        };
        struct {
            float m33[3][3];
        };
        struct {
            float m9[9];
        };
        struct {
            float mf0; float mf1; float mf2;
            float mf4; float mf5; float mf6;
            float mf8; float mf9; float mf10;
        };
        struct {
            float m00; float m01; float m02;
            float m10; float m11; float m12;
            float m20; float m21; float m22;
        };
    };

    __forceinline cMat3() {};
    __forceinline cMat3(bool identity) :
        mf0(1), mf1(0), mf2(0),
        mf4(0), mf5(1), mf6(0),
        mf8(0), mf9(0), mf10(1)
    {
    }
    __forceinline explicit cMat3(float val) :
        mf0(val), mf1(val), mf2(val),
        mf4(val), mf5(val), mf6(val),
        mf8(val), mf9(val), mf10(val)
    {
    }
    __forceinline cMat3(
        float f0, float f1, float f2,
        float f4, float f5, float f6,
        float f8, float f9, float f10) :
        mf0(f0), mf1(f1), mf2(f2),
        mf4(f4), mf5(f5), mf6(f6),
        mf8(f8), mf9(f9), mf10(f10)
    {
    }

    __forceinline cMat3(cVec3 &v0, cVec3 &v1, cVec3 &v2) :
        mRow0(v0), mRow1(v1), mRow2(v2)
    {}

    __forceinline cMat3(const cMat3 &m) :
        mRow0(m.mRow0), mRow1(m.mRow1), mRow2(m.mRow2)
    {}

    __forceinline ~cMat3() {}

    __forceinline cMat3 & operator = (const cMat3& v) { mRow0 = v.mRow0; mRow1 = v.mRow1; mRow2 = v.mRow2; return *this; }
    __forceinline cMat3 Transposed(void) const;

    cMat3 Inverseed() const;

    float Determenent() const
    {
        return
            m00 * (m11 * m22 - m21 * m12) -
            m10 * (m01 * m22 - m21 * m02) +
            m20 * (m01 * m12 - m11 * m02);
    }

    void SetValue(
        const float& xx, const float& xy, const float& xz,
        const float& yx, const float& yy, const float& yz,
        const float& zx, const float& zy, const float& zz)
    {
        mRow0.Set(xx, xy, xz);
        mRow1.Set(yx, yy, yz);
        mRow2.Set(zx, zy, zz);
    }
};

class 
    cMat4
{
public:
    union {
        struct {
            cVec4 mRow[4];
        };
        struct {
            cVec4 mRow0;
            cVec4 mRow1;
            cVec4 mRow2;
            cVec4 mRow3;
        };
        struct {
            float m44[4][4];
        };
        struct {
            float m16[16];
        };
        struct {
            float mf0; float mf1; float mf2; float mf3;
            float mf4; float mf5; float mf6; float mf7;
            float mf8; float mf9; float mf10; float mf11;
            float mf12; float mf13; float mf14; float mf15;
        };
        struct {
            float m00; float m01; float m02; float m03;
            float m10; float m11; float m12; float m13;
            float m20; float m21; float m22; float m23;
            float m30; float m31; float m32; float m33;
        };
    };

    __forceinline cMat4() {};
    __forceinline explicit cMat4(bool identity) :
        mf0(1), mf1(0), mf2(0), mf3(0),
        mf4(0), mf5(1), mf6(0), mf7(0),
        mf8(0), mf9(0), mf10(1), mf11(0),
        mf12(0), mf13(0), mf14(0), mf15(1)
    {
    }
    __forceinline explicit cMat4(float val) :
        mf0(val), mf1(val), mf2(val), mf3(val),
        mf4(val), mf5(val), mf6(val), mf7(val),
        mf8(val), mf9(val), mf10(val), mf11(val),
        mf12(val), mf13(val), mf14(val), mf15(val)
    {
    }
    __forceinline cMat4(
        float f0, float f1, float f2, float f3,
        float f4, float f5, float f6, float f7,
        float f8, float f9, float f10, float f11,
        float f12, float f13, float f14, float f15) :
        mf0(f0), mf1(f1), mf2(f2), mf3(f3),
        mf4(f4), mf5(f5), mf6(f6), mf7(f7),
        mf8(f8), mf9(f9), mf10(f10), mf11(f11),
        mf12(f12), mf13(f13), mf14(f14), mf15(f15)

    {
    }
    __forceinline cMat4(
        float f[]) :
        mf0(f[0]), mf1(f[1]), mf2(f[2]), mf3(f[3]),
        mf4(f[4]), mf5(f[5]), mf6(f[6]), mf7(f[7]),
        mf8(f[8]), mf9(f[9]), mf10(f[10]), mf11(f[11]),
        mf12(f[12]), mf13(f[13]), mf14(f[14]), mf15(f[15])
    {
    }
    __forceinline cMat4(const cVec4 &v0, const cVec4 &v1, const cVec4 &v2, const cVec4 &v3) :
        mRow0(v0), mRow1(v1), mRow2(v2), mRow3(v3)
    {}
    __forceinline cMat4(const cMat4 &m) :
        mRow0(m.mRow0), mRow1(m.mRow1), mRow2(m.mRow2), mRow3(m.mRow3)
    {}
    __forceinline ~cMat4() {}

    __forceinline cMat4 & operator = (const cMat4& v) { mRow0 = v.mRow0; mRow1 = v.mRow1; mRow2 = v.mRow2; mRow3 = v.mRow3;  return *this; }

    __forceinline cMat4 ToD3DProjection(void);

    __forceinline cMat4 Transposed(void) const;

    __forceinline void TranslateLocal(
        const cVec3 &v
        );

    __forceinline void TranslateLocal(
        const cVec4 &v
    );

    __forceinline void SetTranslation(
        const cVec3 &v
    );

    __forceinline void SetTranslation(
        const cVec4 &v
    );

    cMat4 Inverse() const;

    float Determenent() const;

    // helper functions
    static cMat4 rotMatX(const float angle);
    static cMat4 rotMatY(const float angle);
    static cMat4 rotMatZ(const float angle);
    static cMat4 rotMatXY(const float angleX, const float angleY);
    static cMat4 rotMatZXY(const float angleX, const float angleY, const float angleZ);
    static cMat4 transMat(const cVec4 &v);
    static cMat4 transMat(const float x, const float y, const float z);
    static cMat4 scaleMat(const float x, const float y, const float z);
    static cMat4 perspProjMat(const float fov, const float zNear, const float zFar);
    static cMat4 perspProjMatX(const float fov, const float AspectRatio, const float zNear, const float zFar);
    static cMat4 perspProjMatY(const float fov, const int width, const int height, const float zNear, const float zFar);
    static cMat4 orthoProjMatX(const float left, const float right, const float top, const float bottom, const float zNear, const float zFar);
    static cMat4 cubeViewMat(const unsigned int side);
    static cMat4 cubeProjMatGL(const float zNear, const float zFar);
    static cMat4 cubeProjMatD3D(const float zNear, const float zFar);
    static cMat4 identMat(void);
    static cMat4 LookAt(const cVec3 & Direction, const cVec3 &Up);

    void SetValue(
        const float& xx, const float& xy, const float& xz,
        const float& yx, const float& yy, const float& yz,
        const float& zx, const float& zy, const float& zz)
    {
        mRow0.Set(xx, xy, xz, 0.0f);
        mRow1.Set(yx, yy, yz, 0.0f);
        mRow2.Set(zx, zy, zz, 0.0f);
        mRow3.Set(0.0f, 0.0f, 0.0f, 0.0f);
    }

    void GetEulerZYX(float& yaw, float& pitch, float& roll, unsigned int solution_number = 1) const
    {
        // first use the normal calculus
        yaw = float(atan2f(mRow[1].x, mRow[0].x));
        pitch = float(asinf(-mRow[2].x));
        roll = float(atan2f(mRow[2].y, mRow[2].z));

        // on pitch = +/-HalfPI
        if (fabsf(pitch) == Math::HalfPi)
        {
            if (yaw > 0)
                yaw -= Math::Pi;
            else
                yaw += Math::Pi;

            if (roll > 0)
                roll -= Math::Pi;
            else
                roll += Math::Pi;
        }
    }
};

__forceinline cMat4 operator + (const cMat4 &m, const cMat4 &n);
__forceinline cMat4 operator - (const cMat4 &m, const cMat4 &n);
__forceinline cMat4 operator - (const cMat4 &m);

__forceinline cMat4 operator * (const cMat4 &m, const cMat4 &n);
__forceinline cVec4 operator * (const cMat4 &m, const cVec4 &v);
__forceinline cVec4 operator * (const cVec4 &v, const cMat4 &m);
__forceinline cMat4 operator * (const cMat4 &m, const float x);

__forceinline cVec3 operator * (const cMat3 &m, const cVec3 &v);
__forceinline cVec3 operator * (const cVec3 &v, const cMat3 &m);

__forceinline cMat4 operator ! (const cMat4 &m);

#include "cMat.inl"

#endif
