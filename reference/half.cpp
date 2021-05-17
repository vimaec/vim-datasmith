#include "half.h"
#include "cVec.h"

#define WIN32
#ifdef WIN32
#include <intrin.h>
#endif

namespace libEngine
{
    float half::m_LookupTable[65536];

    half::half(const float x)
    {
        union
        {
            float floatI;
            unsigned int i;
        };
        floatI = x;

        int e = ((i >> 23) & 0xFF) - 112;
        int m = i & 0x007FFFFF;

        sh = (i >> 16) & 0x8000;
        if (e <= 0)
        {
            // Denorm
            m = ((m | 0x00800000) >> (1 - e)) + 0x1000;
            sh |= (m >> 13);
        }
        else if (e == 143)
        {
            sh |= 0x7C00;
            if (m != 0)
            {
                // NAN
                m >>= 13;
                sh |= m | (m == 0);
            }
        }
        else
        {
            m += 0x1000;
            if (m & 0x00800000)
            {
                // Mantissa overflow
                m = 0;
                e++;
            }
            if (e >= 31)
            {
                // Exponent overflow
                sh |= 0x7C00;
            }
            else
            {
                sh |= (e << 10) | (m >> 13);
            }
        }
    }

    half::operator float() const
    {
        return m_LookupTable[sh];
    }

#if 0
    float Convert(unsigned short sh)
    {
        union
        {
            unsigned int s;
            float result;
        };

        s = (sh & 0x8000) << 16;
        unsigned int e = (sh >> 10) & 0x1F;
        unsigned int m = sh & 0x03FF;

        if (e == 0)
        {
            // +/- 0
            if (m == 0) return result;

            // Denorm
            while ((m & 0x0400) == 0)
            {
                m += m;
                e--;
            }
            e++;
            m &= ~0x0400;
        }
        else if (e == 31)
        {
            // INF / NAN
            s |= 0x7F800000 | (m << 13);
            return result;
        }

        s |= ((e + 112) << 23) | (m << 13);

        return result;
    }

#else

    float Convert(unsigned short sh)
    {
        union
        {
            unsigned int s;
            float result;
        };

        s = (sh & 0x8000) << 16;
        unsigned int e = (sh >> 10) & 0x1F;
        unsigned int m = sh & 0x03FF;

        if (e == 0)
        {
            // Denorm
            unsigned long n;
#ifdef WIN32
            if (!_BitScanReverse(&n, m))
                return 0.0f;
#else
            if (m == 0)
                return 0.0f;
            n = 31 - __builtin_clz(m);
#endif

            n = 10 - n;
            e -= n;
            m <<= n;

            ++e;
            m &= ~0x0400;
        }
        else if (e == 31)
        {
            // INF / NAN
            s |= 0x7F800000 | (m << 13);
            return result;
        }

        s |= ((e + 112) << 23) | (m << 13);

        return result;
    }
#endif

    void half::Initialize()
    {
        unsigned int* dest = (unsigned int*)m_LookupTable;

        // Zero
        dest[0] = 0;

        // Denorm
        for (unsigned int i = 1; i < 0x400; i++)
        {
            unsigned long e;
#ifdef WIN32
            _BitScanReverse(&e, i);
#else
            e = 31 - __builtin_clz(i);
#endif

            dest[i] = ((103 + e) << 23) | ((i << (23 - e)) & 0x7FFFFF);
        }

        // Regular numbers
        for (unsigned int i = 0x400; i < 0x7C00; i++)
        {
            unsigned int e = (i >> 10) & 0x1F;
            unsigned int m = i & 0x03FF;

            dest[i] = ((e + 112) << 23) | (m << 13);
        }

        // INF / NAN
        for (unsigned int i = 0x7C00; i < 0x8000; i++)
        {
            unsigned int m = i & 0x03FF;

            dest[i] = 0x7F800000 | (m << 13);
        }

        // Negative
        for (unsigned int i = 0x8000; i < 0x10000; i++)
        {
            //table1[i] = -table1[i - 32768];
            dest[i] = dest[i - 0x8000] | 0x80000000;
        }
    }

    // This is here to initialize the lookup table automatically
    struct HalfInitializer
    {
        HalfInitializer()
        {
            half::Initialize();
        }
    };

    HalfInitializer g_Hi;


    //////////////////////////////////////////////////////////////////////////

    half4::half4(const half _x, const half _y, const half _z, const half _w)
    {
        x.sh = _x.sh;
        y.sh = _y.sh;
        z.sh = _z.sh;
        w.sh = _w.sh;
    }

    half4::half4(const float _x, const float _y, const float _z, const float _w)
    {
        x = half(_x);
        y = half(_y);
        z = half(_z);
        w = half(_w);
    }

    half4::half4(const cVec4& v) :
        half4(v.x, v.y, v.z, v.w)
    {
    }

    half4::half4(const cVec3& v) :
        half4(v.x, v.y, v.z, 0.0f)
    {
    }

}