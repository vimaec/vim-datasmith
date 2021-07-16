#ifndef __CVEC_H__
#define __CVEC_H__

#include <math.h>
#include "Utils.h"

class cVec2Int;

class cVec2
{
public:
    float x;
    float y;

    cVec2()                   :x( 0 ), y( 0 ) {};
    cVec2(float t)            :x( t ), y( t ) {};
    cVec2(float _x, float _y) :x(_x ), y(_y ) {};
    cVec2(const cVec2& v)     :x(v.x), y(v.y) {};
    cVec2(const cVec2Int& v);

    // Methods
    void    Set            ( float _x, float _y ) { x = _x; y = _y; }
//    cVec2    &    Set                ( const cVec2& v        )        { x = v.x; y = v.y;                        return *this;    }
    float   Length         () const { return sqrtf(x * x + y * y ); }
    float   LengthSqr      () const { return x * x + y * y; }
    float   GetMaxComponent() const { return Utils::Max ( x, y ); }
    bool    IsZero         () const { return (x == 0 && y == 0 ); }
    void    Normalise      ()       { if (!IsZero()) { const float l = 1.0f / Length(); *this *= l; } }
    cVec2   Normalised     () const { if (!IsZero()) { const float l = 1.0f / Length(); return *this * l; } else return *this; }
    cVec2   Reciprical     () const { return cVec2(1.0f / x, 1.0f / y); }
    cVec2   Abs            () const { return cVec2((float)fabs( x ), (float)fabs( y ) ); }

    cVec2   operator +      () const  { return *this; }
    cVec2   operator -      () const  { return cVec2(-x, -y); }

    cVec2 & operator =      ( const cVec2& v ) { x = v.x; y = v.y; return *this; }

    cVec2 & operator +=     ( const cVec2& v ) { x += v.x; y += v.y; return *this; }
    cVec2 & operator -=     ( const cVec2& v ) { x -= v.x; y -= v.y; return *this; }
    cVec2 & operator /=     ( const cVec2& v ) { x /= v.x; y /= v.y; return *this; }
    cVec2 & operator *=     ( const float d )  { x *= d; y *= d; return *this; }
    cVec2 & operator /=     (       float d )  { d = 1.0f / d; x *= d; y *= d; return *this; }

    cVec2   operator +      ( const cVec2& v ) const { return cVec2( x + v.x, y + v.y ); }
    cVec2   operator -      ( const cVec2& v ) const { return cVec2( x - v.x, y - v.y ); }
    cVec2   operator /      ( const cVec2& v ) const { return cVec2( x / v.x, y / v.y ); }
    cVec2   operator *      ( const float d )  const { return cVec2( x * d, y * d); }
    cVec2   operator /      (       float d )  const { d = 1.0f / d; return cVec2( x * d, y * d); }

    float   operator *      ( const cVec2& v ) const { return (x * v.x + y * v.y ); }
    cVec2   operator %      ( const cVec2& v ) const { return cVec2(x * v.x, y * v.y); }
//    cVec2        operator    ^    ( const cVec2& v        ) const { return cVec2((y * v.z) - (z * v.y), (z * v.x) - (x * v.z), (x * v.y) - (y * v.x)); }
    float   operator []     ( int i ) const { return ((float*)&x)[i]; }
    float & operator []     ( int i )       { return ((float*)&x)[i]; }

};


class cVec3;

class cVec2Int
{
public:
    int32_t x;
    int32_t y;

    // Constructors
    cVec2Int()                       :x( 0 ), y( 0 ) {};    
    cVec2Int( cVec2 & v);
    cVec2Int(int32_t t)              :x( t ), y( t ) {};    
    cVec2Int(int32_t _x, int32_t _y) :x(_x ), y(_y ) {};    
    cVec2Int(const cVec2Int& v)      :x(v.x), y(v.y) {};    
    cVec2Int(const cVec2& v)         :x((int32_t)v.x), y((int32_t)v.y) {};    

    // Methods
    void       Set            (int32_t _x, int32_t _y ) { x = _x; y = _y; }
//    cVec2Int    &    Set                (const cVec2Int& v              )          { x = v.x; y = v.y;                        return *this;    }
    int32_t    Length         () const { return (int32_t)sqrtf((float)(x * x + y * y)); }
    int32_t    LengthSqr      () const { return x * x + y * y; }
    int32_t    GetMaxComponent() const { return Utils::Max ( x, y ); }
    bool       IsZero         () const { return (x == 0 && y == 0); }
    void       Normalise      ()       { if (!IsZero()) {*this /= Length(); } }
    cVec2Int   Normalised     () const { if (!IsZero()) { return *this / Length(); } else return *this; }
//    cVec2Int        Reciprical            (                          ) const { return cVec2Int(1.0f / x, 1.0f / y); }
    cVec2Int   Abs            () const { return cVec2Int((int)fabs( x ), (int)fabs( y ) ); }

    cVec2Int   operator + () const { return *this; }
    cVec2Int   operator - () const { return cVec2Int(-x, -y); }

    cVec2Int & operator = ( const cVec2Int& v ) { x = v.x; y = v.y; return *this; }

    cVec2Int & operator += ( const cVec2Int& v ) { x += v.x; y += v.y; return *this; }
    cVec2Int & operator -= ( const cVec2Int& v ) { x -= v.x; y -= v.y; return *this; }
    cVec2Int & operator *= ( const int32_t d )   { x *= d; y *= d; return *this; }
    cVec2Int & operator /= (       int32_t d )   { x /= d; y /= d; return *this; }

    cVec2Int   operator + ( const cVec2Int& v ) const { return cVec2Int( x + v.x, y + v.y ); }
    cVec2Int   operator - ( const cVec2Int& v ) const { return cVec2Int( x - v.x, y - v.y ); }
    cVec2Int   operator * ( const int32_t d )   const { return cVec2Int( x * d, y * d ); }
    cVec2Int   operator / (       int32_t d )   const { return cVec2Int( x / d, y / d ); }

    bool operator == ( const cVec2Int &other ) const { return x == other.x && y == other.y; }

    int32_t    operator *  ( const cVec2Int& v ) const { return (x * v.x + y * v.y); }
    cVec2Int   operator %  ( const cVec2Int& v ) const { return cVec2Int(x * v.x, y * v.y); }
    int32_t    operator [] ( int32_t i ) const { return ((int32_t*)&x)[i]; }
    int32_t &  operator [] ( int32_t i )       { return ((int32_t*)&x)[i]; }
};

inline cVec2::cVec2(const cVec2Int& v) :
    x ( (float)v.x ),
    y ( (float)v.y )
{
}


inline cVec2Int::cVec2Int( cVec2 & v) :
    x( (int)v.x ), 
    y( (int)v.y )
{};    

class cVec3Int;
class cVec4;

class cVec3
{
public:
    float x;
    float y;
    float z;

    // Constructors
    __forceinline cVec3()                               :x( 0 ), y( 0 ), z( 0 ) {};    
    __forceinline cVec3(float t)                        :x( t ), y( t ), z( t ) {};    
    __forceinline cVec3(float _x, float _y, float _z)   :x(_x ), y(_y ), z(_z ) {};    
    __forceinline cVec3(const cVec2& v, float Z = 0.0f) :x(v.x), y(v.y), z(Z)   {};    
    __forceinline cVec3(const cVec3& v)                 :x(v.x), y(v.y), z(v.z) {};    
    __forceinline cVec3(const cVec3Int& v);
    __forceinline cVec3(const cVec4& v);

    // Destructor
    __forceinline ~cVec3() {}

    // Methods
    __forceinline cVec2   ToVecXY        () { return cVec2 ( x, y ); }
    __forceinline cVec2   ToVecYX        () { return cVec2 ( y, x ); }
    __forceinline cVec2   ToVecXZ        () { return cVec2 ( x, z ); }
    __forceinline cVec2   ToVecYZ        () { return cVec2 ( y, z ); }
    __forceinline void    Set            (float _x, float _y, float _z) { x = _x; y = _y; z = _z; }
//    __forceinline cVec3    &    Set                (const cVec3& v              )          { x = v.x; y = v.y; z = v.z;                        return *this;    }
    __forceinline float   Length         () const { return sqrtf(x * x + y * y + z * z); }
    __forceinline float   LengthSqr      () const { return x * x + y * y + z * z; }
    __forceinline float   GetMaxComponent() const { return Utils::Max ( x, Utils::Max ( y, z ) ); }
    __forceinline bool    IsZero         () const { return (x == 0 && y == 0 && z == 0); }
    __forceinline bool    IsNan() const { return (isnan(x) || isnan(y) || isnan(z)); }
    __forceinline bool    IsInf() const { return (isinf(x) || isinf(y) || isinf(z)); }
    __forceinline void    Normalise() { if (!IsZero()) { const float l = 1.0f / Length(); *this *= l; } }
    __forceinline cVec3   Normalised     () const { if (!IsZero()) { const float l = 1.0f / Length(); return *this * l; } else return *this; }
    __forceinline cVec3   Reciprical     () const { return cVec3(1.0f / x, 1.0f / y, 1.0f / z); }
    __forceinline cVec3&  Floor          ()       { x = floorf ( x ); y = floorf ( y ); z = floorf ( z ); return *this; }
    __forceinline cVec3   Abs            () const { return cVec3((float)fabs( x ), (float)fabs( y ), (float)fabs( z ) ); }
    __forceinline cVec3   Max            ( float d ) const { return cVec3( Utils::Max( x, d ), Utils::Max( y, d ), Utils::Max( z, d ) ); }
    __forceinline cVec3   Sqrt            () const { return cVec3( sqrtf(x), sqrtf(y), sqrtf(z)); }

    __forceinline cVec3   Max(const cVec3& Other) const { return cVec3(Utils::Max(x, Other.x), Utils::Max(y, Other.y), Utils::Max(z, Other.z)); }
    __forceinline cVec3   Min(const cVec3& Other) const { return cVec3(Utils::Min(x, Other.x), Utils::Min(y, Other.y), Utils::Min(z, Other.z)); }

    __forceinline cVec3   operator +     () const { return *this; }
    __forceinline cVec3   operator -     () const { return cVec3(-x, -y, -z); }

    __forceinline cVec3 & operator =  ( const cVec3& v )       { x = v.x; y = v.y; z = v.z; return *this; }
    __forceinline bool    operator == ( const cVec3& v ) const { return x == v.x && y == v.y && z == v.z; }
    __forceinline bool    operator != ( const cVec3& v ) const { return x != v.x || y != v.y || z != v.z; }

    __forceinline cVec3 & operator += ( const cVec3& v ) { x += v.x; y += v.y; z += v.z; return *this; }
    __forceinline cVec3 & operator -= ( const cVec3& v ) { x -= v.x; y -= v.y; z -= v.z; return *this; }
    __forceinline cVec3 & operator *= ( const float d )  { x *= d; y *= d; z *= d; return *this; }
    __forceinline cVec3 & operator /= (       float d )  { d = 1.0f / d; x *= d; y *= d; z *= d; return *this; }

    __forceinline cVec3   operator +  ( const cVec3& v ) const { return cVec3( x + v.x, y + v.y, z + v.z ); }
    __forceinline cVec3   operator -  ( const cVec3& v ) const { return cVec3( x - v.x, y - v.y, z - v.z ); }
    __forceinline cVec3   operator *  ( const float d )  const { return cVec3( x * d, y * d, z * d ); }
    __forceinline cVec3   operator /  (       float d )  const { d = 1.0f / d; return cVec3( x * d, y * d, z * d ); }

    __forceinline float   operator *  ( const cVec3& v ) const { return (x * v.x + y * v.y + z * v.z); }
    __forceinline float   operator /  ( const cVec3& v ) const { return (x / v.x + y / v.y + z * v.z); }
    __forceinline cVec3   operator %  ( const cVec3& v ) const { return cVec3(x * v.x, y * v.y, z * v.z); }
    __forceinline cVec3   operator |  ( const cVec3& v ) const { return cVec3(x / v.x, y / v.y, z / v.z); }
    __forceinline cVec3   operator ^  ( const cVec3& v ) const { return cVec3((y * v.z) - (z * v.y), (z * v.x) - (x * v.z), (x * v.y) - (y * v.x)); }
    __forceinline float   operator [] ( int i ) const { return ((float*)&x)[i]; }
    __forceinline float & operator [] ( int i )       { return ((float*)&x)[i]; }

    __forceinline cVec3 &Scale ( cVec3 &Scale ) { x *= Scale.x; y *= Scale.y; z *= Scale.z; return *this; };
    __forceinline cVec3 &Scale ( float _x, float _y, float _z ) { x *= _x; y *= _y; z *= _z; return *this; };
};


class cVec3Int
{
public:
    int32_t x;
    int32_t y;
    int32_t z;

    // Constructors
    __forceinline cVec3Int()                                   :x( 0 ), y( 0 ), z( 0 ) {};
    __forceinline cVec3Int( cVec3 & v);
    __forceinline cVec3Int(int32_t t)                          :x( t ), y( t ), z( t ) {};
    __forceinline cVec3Int(int32_t _x, int32_t _y, int32_t _z) :x(_x ), y(_y ), z(_z ) {};
    __forceinline cVec3Int(const cVec3Int& v)                  :x(v.x), y(v.y), z(v.z) {};
    __forceinline cVec3Int(const cVec3& v)                     :x((int32_t)v.x), y((int32_t)v.y), z((int32_t)v.z) {};

    // Methods
    __forceinline void       Set            (int32_t _x, int32_t _y, int32_t _z) { x = _x; y = _y; z = _z; }
//    __forceinline cVec3Int    &    Set                (const cVec3Int& v              )          { x = v.x; y = v.y; z = v.z;                        return *this;    }
    __forceinline int32_t    Length         () const { return (int32_t)sqrtf((float)(x * x + y * y + z * z)); }
    __forceinline int32_t    LengthSqr      () const { return x * x + y * y + z * z; }
    __forceinline int32_t    GetMaxComponent() const { return Utils::Max ( x, Utils::Max ( y, z ) ); }
    __forceinline bool       IsZero         () const { return (x == 0 && y == 0 && z == 0); }
    __forceinline void       Normalise      ()       { if (!IsZero()) {*this /= Length(); } }
    void Saturate() {
        x = Utils::Saturate(x);
        y = Utils::Saturate(y);
        z = Utils::Saturate(z);
    }
    __forceinline cVec3Int   Normalised() const { if (!IsZero()) { return *this / Length(); } else return *this; }
//    __forceinline cVec3Int        Reciprical            (                          ) const { return cVec3Int(1.0f / x, 1.0f / y, 1.0f / z); }
    __forceinline cVec3Int   Abs       () const { return cVec3Int((int)fabs( x ), (int)fabs( y ), (int)fabs( z ) ); }

    __forceinline cVec3Int   operator + () const { return *this; }
    __forceinline cVec3Int   operator - () const { return cVec3Int(-x, -y, -z); }

    __forceinline cVec3Int & operator =  (const cVec3Int& v) { x = v.x; y = v.y; z = v.z; return *this; }
    __forceinline bool       operator == (const cVec3Int& v) { return x == v.x && y == v.y && z == v.z; }

    __forceinline cVec3Int & operator += ( const cVec3Int& v ) { x += v.x; y += v.y; z += v.z; return *this; }
    __forceinline cVec3Int & operator -= ( const cVec3Int& v ) { x -= v.x; y -= v.y; z -= v.z; return *this; }
    __forceinline cVec3Int & operator *= ( const int32_t d )   { x *= d; y *= d; z *= d; return *this; }
    __forceinline cVec3Int & operator /= (       int32_t d )   { x /= d; y /= d; z /= d; return *this; }

    __forceinline cVec3Int   operator + ( const cVec3Int& v ) const { return cVec3Int( x + v.x, y + v.y, z + v.z ); }
    __forceinline cVec3Int   operator - ( const cVec3Int& v ) const { return cVec3Int( x - v.x, y - v.y, z - v.z ); }
    __forceinline cVec3Int   operator * ( const int32_t d )   const { return cVec3Int( x * d, y * d, z * d ); }
    __forceinline cVec3Int   operator / (       int32_t d )   const { return cVec3Int( x / d, y / d, z / d ); }

    __forceinline bool operator == ( const cVec3Int &other ) const { return x == other.x && y == other.y && z == other.z; }

    __forceinline int32_t    operator *  ( const cVec3Int& v ) const { return (x * v.x + y * v.y + z * v.z); }
    __forceinline cVec3Int   operator %  ( const cVec3Int& v ) const { return cVec3Int(x * v.x, y * v.y, z * v.z); }
    __forceinline cVec3Int   operator ^  ( const cVec3Int& v ) const { return cVec3Int((y * v.z) - (z * v.y), (z * v.x) - (x * v.z), (x * v.y) - (y * v.x)); }
    __forceinline int32_t    operator [] ( int32_t i ) const { return ((int32_t*)&x)[i]; }
    __forceinline int32_t &  operator [] ( int32_t i )       { return ((int32_t*)&x)[i]; }

};

inline cVec3::cVec3(const cVec3Int & v) : x((float)v.x), y((float)v.y), z((float)v.z) {};

class cVec4
{
public:
    float x;
    float y;
    float z;
    float w;

    // Constructors
    cVec4()                                       :x( 0 ), y( 0 ), z( 0 ), w(0.0f) {};
    cVec4(float t)                                :x( t ), y( t ), z( t ), w( t )  {};
    cVec4(float _x, float _y, float _z, float _w) :x(_x ), y(_y ), z(_z ), w(_w )  {};
    cVec4(const cVec4& v)                         :x(v.x), y(v.y), z(v.z), w(v.w)  {};
    cVec4(const cVec3& v, float w = 1.0f)         :x(v.x), y(v.y), z(v.z), w( w )  {};

    // Destructor
    ~cVec4() {}

    // Methods
    cVec2   toVec2      () { return cVec2 ( x, y ); }
    void    Set         (float _x, float _y, float _z, float _w) { x = _x; y = _y; z = _z; w = _w; }
//    cVec4    &    Set                (const cVec4& v              )          { x = v.x; y = v.y; z = v.z; w = v.w;                        return *this;    }
    float   Length      () const { return (float)sqrt(x * x + y * y + z * z + w * w); }
    float   LengthSqr   () const { return x * x + y * y + z * z + w * w; }
    bool    IsZero      () const { return (x == 0 && y == 0 && z == 0 && w == 0); }
    void    Normalise   ()       { if (!IsZero()) { const float l = 1.0f / Length(); *this *= l; } }
    void    Saturate() {
        x = Utils::Saturate(x); 
        y = Utils::Saturate(y);
        z = Utils::Saturate(z);
        w = Utils::Saturate(w);
    }
    cVec4   Normalised  () const { if (!IsZero()) { const float l = 1.0f / Length(); return *this * l; } else return *this;    }
    cVec4   Reciprical  () const { return cVec4(1.0f / x, 1.0f / y, 1.0f / z, 1.0f / w); }
    cVec4   Abs         () const { return cVec4((float)fabs( x ), (float)fabs( y ), (float)fabs( z ), (float)fabs( w ) ); }

    cVec4   operator +  () const { return *this; }
    cVec4   operator -  () const { return cVec4(-x, -y, -z, -w); }

    cVec4 & operator =  ( const cVec4& v ) { x = v.x; y = v.y; z = v.z; w = v.w; return *this; }

    cVec4 & operator += ( const cVec4& v ) { x += v.x; y += v.y; z += v.z; w += v.w; return *this; }
    cVec4 & operator -= ( const cVec4& v ) { x -= v.x; y -= v.y; z -= v.z; w -= v.w; return *this; }
    cVec4 & operator *= ( const float d )  { x *= d; y *= d; z *= d; w *= d; return *this; }
    cVec4 & operator /= (       float d )  { d = 1.0f / d; x *= d; y *= d; z *= d; w *= d; return *this; }

    cVec4   operator +  ( const cVec4& v ) const { return cVec4( x + v.x, y + v.y, z + v.z, w + v.w ); }
    cVec4   operator -  ( const cVec4& v ) const { return cVec4( x - v.x, y - v.y, z - v.z, w - v.w ); }
    cVec4   operator *  ( const float d )  const { return cVec4( x * d, y * d, z * d, w * d ); }
    cVec4   operator /  (       float d )  const { d = 1.0f / d; return cVec4( x * d, y * d, z * d, w * d ); }

    float   operator *  ( const cVec4& v              ) const 
    { 
        return (x * v.x + y * v.y + z * v.z + w * v.w);        
    }
    cVec4   operator %  ( const cVec4& v ) const { return cVec4(x * v.x, y * v.y, z * v.z, w * v.w); }
//    cVec4        operator    ^    ( const cVec4& v              ) const { return cVec4((y * v.z) - (z * v.y), (z * v.x) - (x * v.z), (x * v.y) - (y * v.x)); }
    float   operator [] ( int i ) const { return ((float*)&x)[i]; }
    float & operator [] ( int i )       { return ((float*)&x)[i]; }

    __forceinline bool operator == (const cVec4& v) const { return x == v.x && y == v.y && z == v.z && w == v.w; }
    __forceinline bool operator != (const cVec4& v) const { return x != v.x || y != v.y || z != v.z || w != v.w; }

    unsigned int toRGBA(void) const {
        return (int(x * 255.0f) | (int(y * 255.0f) << 8) | (int(z * 255.0f) << 16) | (int(w * 255.0f) << 24));
    }

    unsigned int toBGRA(void) const {
        return (int(z * 255.0f) | (int(y * 255.0f) << 8) | (int(x * 255.0f) << 16) | (int(w * 255.0f) << 24));
    }
};

inline cVec3::cVec3(const cVec4& v) :x(v.x), y(v.y), z(v.z) {};    

class byte4
{
public:
    int8_t x = 0;
    int8_t y = 0;
    int8_t z = 0;
    int8_t w = 0;

    byte4() = default;

    byte4(cVec3 v)
    {
        cVec3 a = v.Normalised() * 127.0f;
        x = (int8_t)Utils::Clamp(a.x, -127.0f, 127.0f);
        y = (int8_t)Utils::Clamp(a.y, -127.0f, 127.0f);
        z = (int8_t)Utils::Clamp(a.z, -127.0f, 127.0f);
        w = 0;
    }
    byte4(
        int8_t ix,
        int8_t iy,
        int8_t iz,
        int8_t iw
    )
    {
        x = ix;
        y = iy;
        z = iz;
        w = iw;
    }
};

class ubyte4
{
public:
    uint8_t x = 0;
    uint8_t y = 0;
    uint8_t z = 0;
    uint8_t w = 0;

    ubyte4() = default;

    ubyte4(cVec4 v)
    {
        cVec4 a = v * 255.0f;
        x = (uint8_t)Utils::Clamp(a.x, 0.0f, 255.0f);
        y = (uint8_t)Utils::Clamp(a.y, 0.0f, 255.0f);
        z = (uint8_t)Utils::Clamp(a.z, 0.0f, 255.0f);
        w = (uint8_t)Utils::Clamp(a.w, 0.0f, 255.0f);
    }
    ubyte4(
        uint8_t ix,
        uint8_t iy,
        uint8_t iz,
        uint8_t iw
    )
    {
        x = ix;
        y = iy;
        z = iz;
        w = iw;
    }
};

#endif
