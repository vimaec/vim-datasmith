#ifndef __libEngine_Math_half_h__
#define __libEngine_Math_half_h__

class cVec3;
class cVec4;

namespace libEngine
{
    class half
    {
    public:
        unsigned short sh;

        half() {}
        half(const float x);

        operator float() const;

        static inline float Convert(const unsigned short sh) { return m_LookupTable[sh]; }

    private:
        friend struct HalfInitializer;

        static void Initialize();
        static float m_LookupTable[65536];
    };

    struct half4
    {
        half x, y, z, w;

            // Constructors
        half4() {}
        half4(const half _x, const half _y, const half _z, const half _w);
        half4(const float _x, const float _y, const float _z, const float _w);
        half4(const cVec4& v);
        half4(const cVec3& v);
    };

}

#endif
